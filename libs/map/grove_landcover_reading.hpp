#pragma once

#include "map/features_fetcher.hpp"

#include "indexer/feature_decl.hpp"

#include "geometry/rect2d.hpp"

#include <algorithm>
#include <vector>

namespace grove
{
// Organic Maps' country maps index a feature only from the zoom where upstream's style starts drawing it, so zoom 11
// reads zoom 12's index too: grass, meadows, heath and scrub, which Grove's style colours there (zoom 12 geometry
// level, which these features have; zoom 10 uses the zoom 10 level, which they lack). Features without drawing rules
// at the current zoom are dropped by RuleDrawer right after their types are read.
inline int ExtraIndexScale(int scale)
{
  return scale == 11 ? 12 : -1;
}

template <typename Fn>
void ForEachFeatureIDWithExtraIndex(FeaturesFetcher const & fetcher, m2::RectD const & rect, Fn const & fn, int scale)
{
  std::vector<FeatureID> ids;
  auto const collect = [&ids](FeatureID const & id) { ids.push_back(id); };
  fetcher.ForEachFeatureID(rect, collect, scale);
  fetcher.ForEachFeatureID(rect, collect, ExtraIndexScale(scale));

  // Sorting groups the ids by map file, as TileInfo expects, and drops features found by both queries.
  std::sort(ids.begin(), ids.end());
  ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
  for (auto const & id : ids)
    fn(id);
}
}  // namespace grove
