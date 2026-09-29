#pragma once

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/tile_key.hpp"

#include "drape/drape_global.hpp"
#include "drape/pointers.hpp"

#include "geometry/rect2d.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace grove::landcover
{
// Land cover when zoomed out (zoom 11 and out), where Organic Maps' map files have no forests, fields or heath: colours
// from ESA WorldCover 2021 (10 m, CC BY 4.0, https://esa-worldcover.org), recoloured to Grove's palette and drawn
// under the map's areas by drape_frontend/grove_raster_layers.hpp. WorldCover is a set of Cloud-Optimized GeoTIFFs
// on AWS Open Data, one per 3° square, each with overviews down to 600 m pixels: a map tile needs a square's header
// and one tile of the overview that matches its zoom, which HTTP range requests fetch; both are cached on disk.
// Zoom 5 and out come from a bundled pack of class tiles (data/grove_landcover_world.bin,
// tools/grove/landcover_world.py).

// WorldCover classes.
enum Class : uint8_t
{
  kNoData = 0,
  kTrees = 10,
  kShrubs = 20,
  kGrass = 30,
  kCrops = 40,
  kBuilt = 50,
  kBare = 60,
  kSnow = 70,
  kWater = 80,
  kWetland = 90,
  kMangroves = 95,
  kMoss = 100,
};

// RGBA of a class in the light or dark style; transparent for water (the map draws it) and no data.
std::array<uint8_t, 4> ClassColor(uint8_t cls, bool dark);

// The WorldCover file of the 3° square whose south-west corner is at (lat, lon), e.g. "N51E003".
std::string SquareName(int lat, int lon);

// One resolution level of a WorldCover file: its size, tile size and where its tiles are.
struct Level
{
  uint32_t m_width = 0;
  uint32_t m_height = 0;
  uint32_t m_tileWidth = 0;
  uint32_t m_tileHeight = 0;
  std::vector<uint32_t> m_offsets;
  std::vector<uint32_t> m_sizes;
};

// Reads the levels of a little-endian classic TIFF (full resolution first) from its first bytes. readMore(offset,
// size) returns bytes beyond them, for arrays that don't fit. Nothing if it isn't such a TIFF.
std::optional<std::vector<Level>> ParseLevels(std::string const & head,
                                              std::function<std::string(uint32_t, uint32_t)> const & readMore);

// Registers the land cover layer's tile reader; call before the drape engine is created.
void CreateProvider(std::function<ref_ptr<df::DrapeEngine>()> getEngine, std::function<bool()> isDarkStyle);

// Saves the WorldCover tiles the region's map tiles need from zoom 6 to 11 into the disk cache, so the layer works
// offline there (map/grove_offline_layers.hpp). Blocks; false on network errors.
bool Prefetch(m2::RectD const & mercatorRect);

// The switch, Feature::Landcover (on by default). Takes effect at once.
bool IsEnabled();
void SetEnabled(ref_ptr<df::DrapeEngine> engine, bool enabled);
}  // namespace grove::landcover
