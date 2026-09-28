#pragma once

#include "map/features_fetcher.hpp"

#include "indexer/feature_decl.hpp"

#include "geometry/rect2d.hpp"

#include <algorithm>
#include <vector>

namespace grove
{
// Organic Maps' country maps index a feature only from the zoom where upstream's style starts drawing it:
// grass, meadows, heath and scrub from zoom 12, farmland from 14. So the zoomed-out views miss the fields
// that Grove's style colors there. Reading the zoom 12 index too brings them in; features without drawing
// rules at the current zoom are dropped by RuleDrawer right after their types are read.
int constexpr kLandcoverIndexScale = 12;

// Zoom 11 geometry comes from the map's zoom 12 geometry level, which these features have.
// Zoom 10 uses the zoom 10 level, which they lack.
inline bool ReadsLandcoverIndex(int scale)
{
  return scale == 11;
}

template <typename Fn>
void ForEachFeatureIDWithLandcover(FeaturesFetcher const & fetcher, m2::RectD const & rect, Fn const & fn, int scale)
{
  std::vector<FeatureID> ids;
  auto const collect = [&ids](FeatureID const & id) { ids.push_back(id); };
  fetcher.ForEachFeatureID(rect, collect, scale);
  fetcher.ForEachFeatureID(rect, collect, kLandcoverIndexScale);
  // Sorting groups the ids by map file, as TileInfo expects, and drops features found by both queries.
  std::sort(ids.begin(), ids.end());
  ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
  for (auto const & id : ids)
    fn(id);
}
}  // namespace grove
