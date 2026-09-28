#pragma once

#include "map/features_fetcher.hpp"

#include "drape_frontend/grove_brands.hpp"

#include "indexer/feature_decl.hpp"

#include "geometry/rect2d.hpp"

#include <algorithm>
#include <vector>

namespace grove
{
// Organic Maps' country maps index a feature only from the zoom where upstream's style starts drawing it, so some
// zooms read one more zoom's index too. Features without drawing rules at the current zoom are dropped by RuleDrawer
// right after their types are read, except what Grove draws early:
// - Zoom 11 reads zoom 12: grass, meadows, heath and scrub, which Grove's style colours there (zoom 12 geometry
//   level, which these features have; zoom 10 uses the zoom 10 level, which they lack).
// - Zoom 15 reads zoom 16: chains' places, which show their logo a zoom early (drape_frontend/grove_brands.hpp).
inline int ExtraIndexScale(int scale)
{
  switch (scale)
  {
  case 11: return 12;
  case 15: return 16;
  default: return -1;
  }
}

template <typename Fn>
void ForEachFeatureIDWithExtraIndex(FeaturesFetcher const & fetcher, m2::RectD const & rect, Fn const & fn, int scale)
{
  std::vector<FeatureID> ids;
  auto const collect = [&ids](FeatureID const & id) { ids.push_back(id); };
  fetcher.ForEachFeatureID(rect, collect, scale);
  size_t const ownCount = ids.size();
  fetcher.ForEachFeatureID(rect, collect, ExtraIndexScale(scale));

  // At the early logo zoom, what only the next zoom's index has is drawn only as logos.
  auto & borrowed = BorrowedFeatures();
  borrowed.clear();
  if (scale == kEarlyLogoZoom)
  {
    std::sort(ids.begin(), ids.begin() + ownCount);
    for (auto it = ids.begin() + ownCount; it != ids.end(); ++it)
      if (!std::binary_search(ids.begin(), ids.begin() + ownCount, *it))
        borrowed.insert(*it);
  }

  // Sorting groups the ids by map file, as TileInfo expects, and drops features found by both queries.
  std::sort(ids.begin(), ids.end());
  ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
  for (auto const & id : ids)
    fn(id);
}
}  // namespace grove
