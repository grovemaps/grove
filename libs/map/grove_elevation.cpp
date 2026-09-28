#include "map/grove_elevation.hpp"

#include "map/grove_relief.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>

#include "3party/stb_image/stb_image.h"

namespace grove
{
namespace
{
// Same source and cache as the relief layer (grove_relief.cpp, RasterTileProvider).
std::string_view constexpr kElevationTilesUrl = "https://s3.amazonaws.com/elevation-tiles-prod/terrarium/";
std::string_view constexpr kElevationCacheSubdir = "grove_relief";
int constexpr kElevationDownloadZoom = 14;
int constexpr kElevationMinCachedZoom = 8;

std::string ElevationTileName(int z, TilePixel const & t)
{
  return std::to_string(z) + "_" + std::to_string(t.m_x) + "_" + std::to_string(t.m_y);
}

std::optional<double> DecodeElevation(std::string const & png, TilePixel const & t)
{
  int w, h, comp;
  auto * data = stbi_load_from_memory(reinterpret_cast<stbi_uc const *>(png.data()), static_cast<int>(png.size()), &w,
                                      &h, &comp, 3);
  if (!data)
    return {};
  std::optional<double> meters;
  if (w == 256 && h == 256)
  {
    auto const * p = data + (t.m_py * 256 + t.m_px) * 3;
    meters = TerrariumMeters(p[0], p[1], p[2]);
  }
  stbi_image_free(data);
  return meters;
}
}  // namespace

TilePixel ToTilePixel(ms::LatLon const & point, int z)
{
  double const n = 1 << z;
  double const lat = std::clamp(point.m_lat, -85.0511, 85.0511) * std::numbers::pi / 180;
  double const x = (point.m_lon + 180) / 360 * n;
  double const y = (1 - std::asinh(std::tan(lat)) / std::numbers::pi) / 2 * n;
  auto const clampTile = [n](double v) { return static_cast<uint32_t>(std::clamp(v, 0.0, n - 1e-9)); };
  TilePixel t;
  t.m_x = clampTile(x);
  t.m_y = clampTile(y);
  t.m_px = std::min(255u, static_cast<uint32_t>((x - t.m_x) * 256));
  t.m_py = std::min(255u, static_cast<uint32_t>((y - t.m_y) * 256));
  return t;
}

double TerrariumMeters(uint8_t r, uint8_t g, uint8_t b)
{
  return r * 256.0 + g + b / 256.0 - 32768;
}

std::optional<double> GetElevation(ms::LatLon const & point)
{
  std::string const dir = base::JoinPath(GetPlatform().WritableDir(), std::string(kElevationCacheSubdir));
  // Tiles saved for the downloaded maps (grove_offline_layers.hpp) and browsed ones, most detailed first.
  std::string const offlineDir = base::JoinPath(GetPlatform().WritableDir(), std::string(kReliefOfflineSubdir));
  for (int z = kElevationDownloadZoom + 1; z >= kElevationMinCachedZoom; --z)
  {
    auto const t = ToTilePixel(point, z);
    std::string path = base::JoinPath(dir, ElevationTileName(z, t) + ".tile");
    if (!Platform::IsFileExistsByFullPath(path))
      path = base::JoinPath(offlineDir, ElevationTileName(z, t) + ".tile");
    if (!Platform::IsFileExistsByFullPath(path))
      continue;
    try
    {
      std::string png;
      FileReader(path).ReadAsString(png);
      if (auto const meters = DecodeElevation(png, t))
        return meters;
    }
    catch (RootException const & e)
    {
      LOG(LWARNING, ("Can't read", path, e.Msg()));
    }
  }

  if (!IsReliefEnabled())
    return {};
  auto const t = ToTilePixel(point, kElevationDownloadZoom);
  platform::HttpClient request(std::string(kElevationTilesUrl) + std::to_string(kElevationDownloadZoom) + "/" +
                               std::to_string(t.m_x) + "/" + std::to_string(t.m_y) + ".png");
  request.SetTimeout(15);
  std::string png;
  if (!request.RunHttpRequest(png))
    return {};
  auto const meters = DecodeElevation(png, t);
  if (meters && Platform::MkDirChecked(dir))
  {
    try
    {
      FileWriter(base::JoinPath(dir, ElevationTileName(kElevationDownloadZoom, t) + ".tile"))
          .Write(png.data(), png.size());
    }
    catch (RootException const & e)
    {
      LOG(LWARNING, ("Can't cache elevation tile", e.Msg()));
    }
  }
  return meters;
}
}  // namespace grove
