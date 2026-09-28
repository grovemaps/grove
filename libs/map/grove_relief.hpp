#pragma once

#include "map/raster_tile_provider.hpp"

#include "drape_frontend/drape_engine.hpp"

#include "drape/pointers.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace grove
{
// Shaded relief (hillshading) from Terrarium elevation tiles, drawn by drape_frontend/grove_relief.hpp.
// Creates the tile provider and registers it with the renderer; call before the drape engine is created.
std::unique_ptr<RasterTileProvider> CreateReliefProvider(std::function<ref_ptr<df::DrapeEngine>()> getEngine);

// The settings switch (key "GroveRelief", on by default). Takes effect at once.
bool IsReliefEnabled();
void SetReliefEnabled(ref_ptr<df::DrapeEngine> engine, bool enabled);

// Turns a decoded Terrarium tile (RGBA8, rows south to north) into relief shading: dark with alpha on slopes
// facing away from a northwest sun, light with alpha on slopes facing it, transparent on flat ground.
void ShadeRelief(std::vector<uint8_t> & rgba, uint32_t width, uint32_t height, double metersPerPixel,
                 double exaggeration);
}  // namespace grove
