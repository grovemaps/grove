#pragma once

#include "drape_frontend/map_data_provider.hpp"
#include "drape_frontend/tile_background_renderer.hpp"

#include "drape/drape_global.hpp"
#include "drape/pointers.hpp"

#include <string_view>
#include <utility>

namespace grove
{
// Shaded relief (hillshading) over the map. It reuses upstream's raster tile background renderer as a second
// instance in dp::BackgroundMode::Relief, drawn after the 2D layer: its tiles are black (shadow) and white (sunlit)
// with alpha, so they darken and lighten the map under roads' labels and icons. The tiles come from
// map/grove_relief.cpp, which registers its reader here before the drape engine starts.
struct ReliefSource
{
  df::MapDataProvider::TTileBackgroundReadFn m_read;
  df::MapDataProvider::TCancelTileBackgroundReadingFn m_cancel;
};

inline ReliefSource & GetReliefSource()
{
  static ReliefSource source;
  return source;
}

// Image uids of relief tiles start with this, so their tile bindings reach the relief renderer.
std::string_view constexpr kReliefImagePrefix = "relief/";

class ReliefLayer
{
public:
  ReliefLayer()
  {
    auto source = GetReliefSource();
    if (source.m_read)
    {
      m_renderer = make_unique_dp<df::TileBackgroundRenderer>(std::move(source.m_read), std::move(source.m_cancel),
                                                              dp::BackgroundMode::Relief);
    }
  }

  // The renderer that owns tiles of this mode or image.
  ref_ptr<df::TileBackgroundRenderer> Route(ref_ptr<df::TileBackgroundRenderer> background,
                                            dp::BackgroundMode mode) const
  {
    return m_renderer && mode == dp::BackgroundMode::Relief ? make_ref(m_renderer) : background;
  }
  ref_ptr<df::TileBackgroundRenderer> Route(ref_ptr<df::TileBackgroundRenderer> background,
                                            std::string_view imageUid) const
  {
    return m_renderer && imageUid.starts_with(kReliefImagePrefix) ? make_ref(m_renderer) : background;
  }

  void OnUpdateViewport(ref_ptr<dp::GraphicsContext> context, df::CoverageResult const & coverage, int zoomLevel)
  {
    if (m_renderer)
      m_renderer->OnUpdateViewport(context, coverage, zoomLevel);
  }

  void Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng, ScreenBase const & screen,
              int zoomLevel, df::FrameValues const & frameValues)
  {
    if (m_renderer)
      m_renderer->Render(context, mng, screen, zoomLevel, frameValues);
  }

  void ClearContextDependentResources(ref_ptr<dp::GraphicsContext> context)
  {
    if (m_renderer)
      m_renderer->ClearContextDependentResources(context);
  }

  void Reset() { m_renderer.reset(); }

private:
  drape_ptr<df::TileBackgroundRenderer> m_renderer;
};
}  // namespace grove
