#include "map/grove_relief.hpp"

#include "drape_frontend/grove_relief.hpp"

#include "platform/settings.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"

#include <algorithm>
#include <cmath>
#include <deque>
#include <memory>
#include <mutex>
#include <string>

namespace grove
{
namespace
{
std::string_view constexpr kEnabledKey = "GroveRelief";

// Tilezen/Mapzen "Terrarium" elevation tiles on AWS Open Data: worldwide, free, no key. Sources and attribution:
// https://github.com/tilezen/joerd/blob/master/docs/attribution.md
char constexpr kTerrariumUrl[] = "https://s3.amazonaws.com/elevation-tiles-prod/terrarium/{z}/{x}/{y}.png";
int constexpr kMaxDemZoom = 15;

double constexpr kEarthCircumference = 2 * math::pi * 6378137.0;

// Sun from the northwest at 45 degrees, as in classic shaded relief; the same side as the 3D building light.
double constexpr kSunX = -0.5;
double constexpr kSunY = 0.5;
double constexpr kSunZ = 0.70710678;

// Strongest shading, reached on slopes facing straight away from or toward the sun.
double constexpr kMaxShadowAlpha = 0.32;
double constexpr kMaxLightAlpha = 0.2;
// Cool shadows and warm light read softer than black and white.
uint8_t constexpr kShadowColor[] = {38, 48, 70};
uint8_t constexpr kLightColor[] = {255, 252, 240};

// Terrain looks flat when zoomed out, where a pixel covers hundreds of meters; exaggerate it there.
double Exaggeration(int demZoom)
{
  return std::clamp(std::pow(2.0, (12 - demZoom) * 0.35), 1.0, 4.0);
}

// Recently shaded tiles: zoomed in, several map tiles share one elevation tile.
class ShadedCache
{
public:
  std::shared_ptr<std::vector<uint8_t> const> Find(std::string const & uid)
  {
    std::lock_guard lock(m_mutex);
    auto const it = std::ranges::find(m_entries, uid, &Entry::first);
    return it != m_entries.end() ? it->second : nullptr;
  }

  void Add(std::string const & uid, std::shared_ptr<std::vector<uint8_t> const> pixels)
  {
    std::lock_guard lock(m_mutex);
    m_entries.emplace_back(uid, std::move(pixels));
    if (m_entries.size() > 16)
      m_entries.pop_front();
  }

private:
  using Entry = std::pair<std::string, std::shared_ptr<std::vector<uint8_t> const>>;
  std::deque<Entry> m_entries;
  std::mutex m_mutex;
};

double DecodeTerrarium(uint8_t const * px)
{
  // Clamped at sea level: no seabed relief under water.
  return std::max(0.0, px[0] * 256.0 + px[1] + px[2] / 256.0 - 32768.0);
}
}  // namespace

void ShadeRelief(std::vector<uint8_t> & rgba, uint32_t width, uint32_t height, double metersPerPixel,
                 double exaggeration)
{
  CHECK_EQUAL(rgba.size(), size_t{width} * height * 4, ());
  auto const w = static_cast<int>(width);
  auto const h = static_cast<int>(height);

  std::vector<double> elevation(size_t{width} * height);
  for (size_t i = 0; i < elevation.size(); ++i)
    elevation[i] = DecodeTerrarium(&rgba[i * 4]);

  // Neighbours outside the tile repeat the edge.
  auto const e = [&](int x, int y) { return elevation[std::clamp(y, 0, h - 1) * w + std::clamp(x, 0, w - 1)]; };

  double const k = exaggeration / (8 * metersPerPixel);
  for (int y = 0; y < h; ++y)
  {
    for (int x = 0; x < w; ++x)
    {
      // Horn's method; rows run south to north, so y + 1 is north.
      double const dzdx = k * ((e(x + 1, y + 1) + 2 * e(x + 1, y) + e(x + 1, y - 1)) -
                               (e(x - 1, y + 1) + 2 * e(x - 1, y) + e(x - 1, y - 1)));
      double const dzdy = k * ((e(x - 1, y + 1) + 2 * e(x, y + 1) + e(x + 1, y + 1)) -
                               (e(x - 1, y - 1) + 2 * e(x, y - 1) + e(x + 1, y - 1)));
      double const light = (-dzdx * kSunX - dzdy * kSunY + kSunZ) / std::sqrt(dzdx * dzdx + dzdy * dzdy + 1);
      double const delta = light - kSunZ;  // Relative to flat ground.

      uint8_t * px = &rgba[(size_t{static_cast<size_t>(y)} * width + x) * 4];
      double alpha;
      if (delta < 0)
      {
        std::copy(std::begin(kShadowColor), std::end(kShadowColor), px);
        alpha = kMaxShadowAlpha * std::min(1.0, -delta / kSunZ);
      }
      else
      {
        std::copy(std::begin(kLightColor), std::end(kLightColor), px);
        alpha = kMaxLightAlpha * std::min(1.0, delta / (1 - kSunZ));
      }
      px[3] = static_cast<uint8_t>(std::lround(alpha * 255));
    }
  }
}

std::unique_ptr<RasterTileProvider> CreateReliefProvider(std::function<ref_ptr<df::DrapeEngine>()> getEngine)
{
  ReliefEnabled() = IsReliefEnabled();

  RasterTileProvider::Params params;
  params.m_urlTemplate = kTerrariumUrl;
  params.m_cacheSubdir = "grove_relief";
  params.m_minZoom = 3;
  params.m_maxZoom = kMaxDemZoom;
  params.m_maxCacheBytes = 200ull * 1024 * 1024;

  auto provider = std::make_unique<RasterTileProvider>(
      std::move(params), [getEngine = std::move(getEngine)](
                             df::TileKey const & tileKey, dp::BackgroundMode mode, std::string const & imageUid,
                             uint32_t width, uint32_t height, m2::RectF const & rect, std::vector<uint8_t> && rgba)
  {
    // Called on a background thread, like upstream's raster tiles; the engine calls only post messages.
    auto engine = getEngine();
    if (!engine)
      return;

    static ShadedCache cache;
    std::string const uid = std::string(kReliefImagePrefix) + imageUid;
    auto shaded = cache.Find(uid);
    if (!shaded)
    {
      // OM zoom Z is web-mercator zoom Z - 1; deeper tiles reuse part of the deepest DEM tile.
      int const demZoom = std::min(static_cast<int>(tileKey.m_zoomLevel) - 1, kMaxDemZoom);
      double const lat = mercator::YToLat(tileKey.GetGlobalRect().Center().y);
      double const metersPerPixel = kEarthCircumference * std::cos(math::DegToRad(lat)) / (width * (1 << demZoom));
      ShadeRelief(rgba, width, height, metersPerPixel, Exaggeration(demZoom));
      shaded = std::make_shared<std::vector<uint8_t> const>(std::move(rgba));
      cache.Add(uid, shaded);
    }

    // Flat ground (and sea) gets no shading: skip the upload and the drawing.
    bool flat = true;
    for (size_t i = 3; i < shaded->size() && flat; i += 4)
      flat = (*shaded)[i] == 0;
    if (flat)
      return;

    engine->AddTileBackgroundImage(uid, width, height, dp::TextureFormat::RGBA8, mode, std::vector<uint8_t>(*shaded));
    engine->SetTileBackgroundData(tileKey, uid, rect);
  });

  RasterTileProvider * p = provider.get();
  GetReliefSource() = {[p](df::TileKey const & key, dp::BackgroundMode mode) { return p->RequestTile(key, mode); },
                       [p](df::TileKey const & key, dp::BackgroundMode mode) { p->CancelTile(key, mode); }};
  return provider;
}

bool IsReliefEnabled()
{
  bool enabled = true;
  settings::TryGet(kEnabledKey, enabled);
  return enabled;
}

void SetReliefEnabled(ref_ptr<df::DrapeEngine> engine, bool enabled)
{
  settings::Set(kEnabledKey, enabled);
  ReliefEnabled() = enabled;
  // Re-reads the visible tiles, which also updates the relief layer's viewport (requests or drops its tiles).
  if (engine)
    engine->InvalidateRect(mercator::Bounds::FullRect());
}
}  // namespace grove
