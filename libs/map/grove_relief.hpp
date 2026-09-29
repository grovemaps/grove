#pragma once

#include "map/raster_tile_provider.hpp"

#include "drape_frontend/drape_engine.hpp"

#include "drape/pointers.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

namespace grove
{
// Shaded relief (hillshading) from Terrarium elevation tiles, drawn by drape_frontend/grove_raster_layers.hpp.
// Creates the tile provider and registers it with the renderer; call before the drape engine is created.
std::unique_ptr<RasterTileProvider> CreateReliefProvider(std::function<ref_ptr<df::DrapeEngine>()> getEngine);

// Terrarium tiles saved for the downloaded maps (map/grove_offline_layers.hpp), under Platform::WritableDir(), named
// like the cache's: "<z>_<x>_<y>.tile".
std::string_view constexpr kReliefOfflineSubdir = "grove_relief_offline";
std::string_view constexpr kTerrariumTiles = "https://s3.amazonaws.com/elevation-tiles-prod/terrarium/";

// The switch, Feature::Relief (on by default). Takes effect at once.
bool IsReliefEnabled();
void SetReliefEnabled(ref_ptr<df::DrapeEngine> engine, bool enabled);

// The elevation tint of ground at this height: RGB and alpha (0..1), clear in the lowlands.
std::array<double, 4> ElevationTint(double meters);

// Turns a decoded Terrarium tile (RGBA8, rows south to north) into relief shading: dark with alpha on slopes
// facing away from a northwest sun, light with alpha on slopes facing it, transparent on flat ground.
void ShadeRelief(std::vector<uint8_t> & rgba, uint32_t width, uint32_t height, double metersPerPixel,
                 double exaggeration);
}  // namespace grove
