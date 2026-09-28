#pragma once

#include "geometry/latlon.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace grove
{
// The height of any point, for the place card: read from the Terrarium elevation tiles the relief layer caches in
// grove_relief/ (grove_relief.hpp), the most detailed one cached first. Without one, and with relief switched on,
// it downloads the zoom 14 tile (about 6 m pixels in Europe) into the same cache. Blocks while downloading.
std::optional<double> GetElevation(ms::LatLon const & point);

// The web mercator tile of a point at zoom z and the pixel in it (256 px tiles, rows north to south).
struct TilePixel
{
  uint32_t m_x = 0, m_y = 0;
  uint32_t m_px = 0, m_py = 0;
};
TilePixel ToTilePixel(ms::LatLon const & point, int z);

// The height a Terrarium pixel encodes: R * 256 + G + B / 256 - 32768 meters.
double TerrariumMeters(uint8_t r, uint8_t g, uint8_t b);
}  // namespace grove
