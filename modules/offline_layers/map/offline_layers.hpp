#pragma once

#include "geometry/rect2d.hpp"

#include "base/thread.hpp"

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>

namespace grove
{
// Saves the relief (to zoom 12, drawn one zoom deeper than the map's) and land cover (zoom 6 to 11) of each downloaded
// map region, so both layers work
// offline there and show sharp at once instead of blurry parents: relief tiles go to grove_relief_offline/, which the
// relief layer reads before downloading and never evicts (modules/relief/map/relief.hpp); land cover to its own disk
// cache (modules/landcover/map/landcover.hpp). Only on Wi-Fi and with the layer switched on; a region is done once per
// map version (marker files in <dir>). Regions left undone, offline or on mobile data, are tried again at the next
// start or download.
class OfflineLayers
{
public:
  explicit OfflineLayers(std::string dir);
  ~OfflineLayers();

  // Queues a map region: its file name, version and mercator rect.
  void Add(std::string const & name, int64_t version, m2::RectD const & rect);

  // The web mercator tiles of a mercator rect at zoom z: x0, y0, x1, y1 (inclusive, rows north to south).
  struct TileRange
  {
    uint32_t m_x0 = 0, m_y0 = 0, m_x1 = 0, m_y1 = 0;
  };
  static TileRange ToTileRange(m2::RectD const & rect, int z);

private:
  struct Region
  {
    std::string m_name;
    int64_t m_version = 0;
    m2::RectD m_rect;
  };

  void Run();
  bool SaveRelief(m2::RectD const & rect);

  std::string const m_dir;
  std::mutex m_mutex;
  std::condition_variable m_cv;
  std::deque<Region> m_queue;
  bool m_stop = false;
  // Attached to the JVM on Android, which HTTP requests need.
  threads::SimpleThread m_worker;
};
}  // namespace grove
