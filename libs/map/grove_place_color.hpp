#pragma once

#include "indexer/feature_decl.hpp"

#include <cstdint>

class DataSource;

namespace grove
{
// The colour of a place's card (Android place page): its chain's logo colour, otherwise its category's label colour
// in the current map style (the colour of its icon circle), as 0xRRGGBB. 0 for places with neither.
uint32_t GetPlaceColor(DataSource const & dataSource, FeatureID const & id);
}  // namespace grove
