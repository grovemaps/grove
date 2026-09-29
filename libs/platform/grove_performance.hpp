#pragma once

#include "platform/grove_features.hpp"

#include <algorithm>

namespace grove
{
// The performance boost (Feature::PerformanceBoost, off by default), read once at start: its enhancements size thread
// pools, so they take effect after a restart.
// - Map tiles are read and turned into geometry by more threads: all cores but two, 3 to 6 (upstream: 2, or 3 from
//   6 cores), which fills the screen faster after a jump or a zoom (drape_frontend/read_manager.cpp).
// - Land cover tiles are downloaded and made by 4 threads instead of 2 (map/grove_landcover.cpp).
inline bool PerformanceBoost()
{
  return IsOn(Feature::PerformanceBoost);
}

inline unsigned BoostedThreads(unsigned cores)
{
  return std::clamp(cores > 2 ? cores - 2 : 1u, 3u, 6u);
}
}  // namespace grove
