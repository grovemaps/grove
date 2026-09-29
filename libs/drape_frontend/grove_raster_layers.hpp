#pragma once

#include "drape_frontend/map_data_provider.hpp"
#include "drape_frontend/tile_background_renderer.hpp"

#include "drape/drape_global.hpp"
#include "drape/pointers.hpp"

#include <atomic>
#include <string_view>
#include <utility>

namespace grove
{
// Grove's raster layers: upstream's raster tile background renderer, one instance per layer, each in its own
// dp::BackgroundMode and drawn at its own place in the frame:
// - land cover (map/grove_landcover.cpp): colours for forests, fields, heath... when zoomed out, where the map files
//   have none; drawn over the background, under the map's areas, which take over when zoomed in.
// - shaded relief (map/grove_relief.cpp): black (shadow) and white (sunlit) with alpha, drawn after the 2D layer,
//   under roads' labels and icons.
// The map side registers each layer's tile reader here before the drape engine starts.
struct RasterLayerSource
{
  df::MapDataProvider::TTileBackgroundReadFn m_read;
  df::MapDataProvider::TCancelTileBackgroundReadingFn m_cancel;
};

inline size_t LayerIndex(dp::BackgroundMode mode)
{
  return mode == dp::BackgroundMode::Landcover ? 1 : 0;
}

inline RasterLayerSource & GetRasterLayerSource(dp::BackgroundMode mode)
{
  static RasterLayerSource sources[2];
  return sources[LayerIndex(mode)];
}

// Settings switches. When off, a layer draws nothing and requests no tiles.
inline std::atomic<bool> & RasterLayerEnabled(dp::BackgroundMode mode)
{
  static std::atomic<bool> enabled[2] = {true, true};
  return enabled[LayerIndex(mode)];
}

// Image uids of a layer's tiles start with its prefix, so their tile bindings reach the layer's renderer.
inline std::string_view ImagePrefix(dp::BackgroundMode mode)
{
  return mode == dp::BackgroundMode::Landcover ? "landcover/" : "relief/";
}

// Land cover shows up to zoom 11, fading out there as the map's own areas take over.
int constexpr kLandcoverMaxZoom = 11;

// Relief tiles are requested one zoom deeper than the map's: a 256 px elevation tile per quarter of a map tile, since
// phones draw a map tile 2 to 3 times larger than 256 px. Land cover makes 512 px tiles instead.
int constexpr kReliefZoomOffset = 1;

class RasterLayer
{
public:
  RasterLayer(dp::BackgroundMode mode, int maxZoom, int zoomOffset = 0)
    : m_mode(mode)
    , m_maxZoom(maxZoom)
    , m_zoomOffset(zoomOffset)
  {
    auto source = GetRasterLayerSource(mode);
    if (source.m_read)
      m_renderer =
          make_unique_dp<df::TileBackgroundRenderer>(std::move(source.m_read), std::move(source.m_cancel), mode);
  }

  bool Owns(dp::BackgroundMode mode) const { return m_renderer && mode == m_mode; }
  bool Owns(std::string_view imageUid) const { return m_renderer && imageUid.starts_with(ImagePrefix(m_mode)); }
  ref_ptr<df::TileBackgroundRenderer> Renderer() { return make_ref(m_renderer); }

  void OnUpdateViewport(ref_ptr<dp::GraphicsContext> context, df::CoverageResult const & coverage, int zoomLevel)
  {
    if (!m_renderer)
      return;
    if (!RasterLayerEnabled(m_mode))
    {
      // Switched off: cancel requests and free the textures.
      if (context != nullptr)
        m_renderer->ClearContextDependentResources(context);
    }
    else if (zoomLevel <= m_maxZoom)
    {
      // Zoomed in further, the layer keeps its tiles for zooming back out.
      df::CoverageResult deeper = coverage;
      deeper.m_minTileX <<= m_zoomOffset;
      deeper.m_maxTileX <<= m_zoomOffset;
      deeper.m_minTileY <<= m_zoomOffset;
      deeper.m_maxTileY <<= m_zoomOffset;
      m_renderer->OnUpdateViewport(context, deeper, zoomLevel + m_zoomOffset);
    }
  }

  void Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng, ScreenBase const & screen,
              int zoomLevel, df::FrameValues const & frameValues)
  {
    if (m_renderer && RasterLayerEnabled(m_mode) && zoomLevel <= m_maxZoom)
      m_renderer->Render(context, mng, screen, zoomLevel, frameValues);
  }

  void ClearContextDependentResources(ref_ptr<dp::GraphicsContext> context)
  {
    if (m_renderer)
      m_renderer->ClearContextDependentResources(context);
  }

  void Reset() { m_renderer.reset(); }

private:
  dp::BackgroundMode const m_mode;
  int const m_maxZoom;
  int const m_zoomOffset;
  drape_ptr<df::TileBackgroundRenderer> m_renderer;
};

class RasterLayers
{
public:
  // The renderer that owns tiles of this mode or image: a Grove layer's, or upstream's background.
  template <typename ModeOrUid>
  ref_ptr<df::TileBackgroundRenderer> Route(ref_ptr<df::TileBackgroundRenderer> background, ModeOrUid const & key)
  {
    for (auto * layer : {&m_landcover, &m_relief})
      if (layer->Owns(key))
        return layer->Renderer();
    return background;
  }

  // Over the background, under the map's areas.
  void RenderUnderMap(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng, ScreenBase const & screen,
                      int zoomLevel, df::FrameValues const & frameValues)
  {
    m_landcover.Render(context, mng, screen, zoomLevel, frameValues);
  }

  // Over areas and roads, under 3D buildings, routes and labels.
  void RenderOverMap(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng, ScreenBase const & screen,
                     int zoomLevel, df::FrameValues const & frameValues)
  {
    m_relief.Render(context, mng, screen, zoomLevel, frameValues);
  }

  void OnUpdateViewport(ref_ptr<dp::GraphicsContext> context, df::CoverageResult const & coverage, int zoomLevel)
  {
    m_landcover.OnUpdateViewport(context, coverage, zoomLevel);
    m_relief.OnUpdateViewport(context, coverage, zoomLevel);
  }

  void ClearContextDependentResources(ref_ptr<dp::GraphicsContext> context)
  {
    m_landcover.ClearContextDependentResources(context);
    m_relief.ClearContextDependentResources(context);
  }

  void Reset()
  {
    m_landcover.Reset();
    m_relief.Reset();
  }

private:
  RasterLayer m_landcover{dp::BackgroundMode::Landcover, kLandcoverMaxZoom};
  RasterLayer m_relief{dp::BackgroundMode::Relief, 20, kReliefZoomOffset};
};
}  // namespace grove
