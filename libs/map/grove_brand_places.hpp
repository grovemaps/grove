#pragma once

#include "drape_frontend/grove_brand_layer.hpp"

#include "indexer/data_source.hpp"

#include "geometry/rect2d.hpp"

#include "base/thread.hpp"

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace grove
{
// The chains' places of each map file, for the logo layer (drape_frontend/grove_brand_layer.hpp). The map files
// index most places only from zoom 16, so the list is made by reading a whole map file once, in the background,
// and cached on disk until the map or the logo pack changes. Until a map's list is ready, its tiles show no logos;
// then onReady redraws the map's area.
class BrandPlaces
{
public:
  BrandPlaces(DataSource const & dataSource, std::string cacheDir, std::function<void(m2::RectD const &)> onReady);
  ~BrandPlaces();

  // Adds a map's chain places in rect to places, or starts making the map's list.
  void ForEachInRect(MwmSet::MwmId const & mwm, m2::RectD const & rect, std::vector<BrandPlace> & places);

  struct Entry
  {
    uint32_t m_index = 0;  // Feature index in the map file.
    m2::PointD m_point;
    std::string m_brand;
  };

  // Reads a map file's chain places, sorted by x. Stops early and returns nothing once stop() is true.
  static std::vector<Entry> Collect(DataSource const & dataSource, MwmSet::MwmId const & mwm,
                                    std::function<bool()> const & stop);

private:
  void Work();
  std::vector<Entry> Load(MwmSet::MwmId const & mwm) const;
  std::string CachePath(MwmSet::MwmId const & mwm) const;

  DataSource const & m_dataSource;
  std::string const m_cacheDir;
  std::function<void(m2::RectD const &)> const m_onReady;

  mutable std::mutex m_mutex;
  std::condition_variable m_wake;
  // Maps without an entry have no list yet; a queued map has an empty one until it is made.
  std::map<MwmSet::MwmId, std::vector<Entry>> m_places;
  std::deque<MwmSet::MwmId> m_queue;
  bool m_stop = false;
  // Attached to the JVM on Android: onReady posts to the UI thread through JNI.
  threads::SimpleThread m_worker;
};
}  // namespace grove
