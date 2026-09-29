#pragma once

#include "platform/settings.hpp"

#include <algorithm>
#include <string_view>

namespace grove
{
// The settings switch "Performance boost" (key "GrovePerformance", off by default), read once at start: its
// enhancements size thread pools, so they take effect after a restart.
// - Map tiles are read and turned into geometry by more threads: all cores but two, 3 to 6 (upstream: 2, or 3 from
//   6 cores), which fills the screen faster after a jump or a zoom (drape_frontend/read_manager.cpp).
// - Land cover tiles are downloaded and made by 4 threads instead of 2 (map/grove_landcover.cpp).
std::string_view constexpr kPerformanceKey = "GrovePerformance";

inline bool PerformanceBoost()
{
  static bool const boost = []
  {
    bool value = false;
    settings::TryGet(kPerformanceKey, value);
    return value;
  }();
  return boost;
}

// The saved switch, for the settings screen: the running app keeps the value it started with.
inline bool SavedPerformanceBoost()
{
  bool value = false;
  settings::TryGet(kPerformanceKey, value);
  return value;
}

inline void SetPerformanceBoost(bool boost)
{
  settings::Set(kPerformanceKey, boost);
}

inline unsigned BoostedThreads(unsigned cores)
{
  return std::clamp(cores > 2 ? cores - 2 : 1u, 3u, 6u);
}
}  // namespace grove
