#include "modules/offline_layers/map/offline_layers.hpp"

#include "modules/core/map/files.hpp"
#include "modules/landcover/map/landcover.hpp"
#include "modules/relief/map/relief.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

#include "geometry/mercator.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <cmath>

namespace grove
{
namespace
{
// Web mercator zooms of the saved relief, which the relief layer shows at OM zooms 5 to 12 (one zoom deeper than the
// map's, see modules/core/drape_frontend/raster_layers.hpp). Closer in, a region would need thousands of tiles.
int constexpr kMinReliefZoom = 5;
int constexpr kMaxReliefZoom = 12;

bool OnWifi()
{
  return GetPlatform().ConnectionStatus() == Platform::EConnectionType::CONNECTION_WIFI;
}

bool WriteFile(std::string const & path, std::string const & bytes)
{
  return WriteAtomically(path, bytes);
}
}  // namespace

OfflineLayers::OfflineLayers(std::string dir) : m_dir(std::move(dir))
{
  UNUSED_VALUE(Platform::MkDirChecked(m_dir));
  m_worker = threads::SimpleThread(&OfflineLayers::Run, this);
}

OfflineLayers::~OfflineLayers()
{
  {
    std::lock_guard lock(m_mutex);
    m_stop = true;
  }
  m_cv.notify_one();
  m_worker.join();
}

void OfflineLayers::Add(std::string const & name, int64_t version, m2::RectD const & rect)
{
  {
    std::lock_guard lock(m_mutex);
    m_queue.push_back({name, version, rect});
  }
  m_cv.notify_one();
}

OfflineLayers::TileRange OfflineLayers::ToTileRange(m2::RectD const & rect, int z)
{
  double const n = 1 << z;
  auto const tile = [n](double v) { return static_cast<uint32_t>(std::clamp(v * n, 0.0, n - 1)); };
  // Mercator x and y run from -180 to 180 here; web tiles count x from the west and y from the north.
  auto const fx = [](double x) { return (x - mercator::Bounds::kMinX) / mercator::Bounds::kRangeX; };
  auto const fy = [](double y) { return (mercator::Bounds::kMaxY - y) / mercator::Bounds::kRangeY; };
  return {tile(fx(rect.minX())), tile(fy(rect.maxY())), tile(fx(rect.maxX())), tile(fy(rect.minY()))};
}

void OfflineLayers::Run()
{
  while (true)
  {
    Region region;
    {
      std::unique_lock lock(m_mutex);
      m_cv.wait(lock, [this] { return m_stop || !m_queue.empty(); });
      if (m_stop)
        return;
      region = std::move(m_queue.front());
      m_queue.pop_front();
    }

    std::string const base = base::JoinPath(m_dir, region.m_name + "_" + std::to_string(region.m_version));
    std::string const reliefDone = base + ".relief", landcoverDone = base + ".landcover6";  // From zoom 6 (before: 7).
    if (IsReliefEnabled() && !Platform::IsFileExistsByFullPath(reliefDone) && OnWifi())
    {
      if (SaveRelief(region.m_rect))
      {
        WriteFile(reliefDone, "");
        LOG(LINFO, ("Saved relief for", region.m_name));
      }
    }
    if (landcover::IsEnabled() && !Platform::IsFileExistsByFullPath(landcoverDone) && OnWifi())
    {
      if (landcover::Prefetch(region.m_rect))
      {
        WriteFile(landcoverDone, "");
        LOG(LINFO, ("Saved land cover for", region.m_name));
      }
    }
  }
}

bool OfflineLayers::SaveRelief(m2::RectD const & rect)
{
  std::string const dir = base::JoinPath(GetPlatform().WritableDir(), std::string(kReliefOfflineSubdir));
  std::string const cacheDir = base::JoinPath(GetPlatform().WritableDir(), "grove_relief");
  if (!Platform::MkDirChecked(dir))
    return false;
  for (int z = kMinReliefZoom; z <= kMaxReliefZoom; ++z)
  {
    auto const range = ToTileRange(rect, z);
    for (uint32_t y = range.m_y0; y <= range.m_y1; ++y)
    {
      for (uint32_t x = range.m_x0; x <= range.m_x1; ++x)
      {
        {
          std::lock_guard lock(m_mutex);
          if (m_stop)
            return false;
        }
        std::string const name = std::to_string(z) + "_" + std::to_string(x) + "_" + std::to_string(y) + ".tile";
        std::string const path = base::JoinPath(dir, name);
        if (Platform::IsFileExistsByFullPath(path))
          continue;
        // Already browsed: move it out of the evicting cache.
        if (std::string const cached = base::JoinPath(cacheDir, name);
            Platform::IsFileExistsByFullPath(cached) && base::RenameFileX(cached, path))
          continue;
        if (!OnWifi())
          return false;
        platform::HttpClient request(std::string(kTerrariumTiles) + std::to_string(z) + "/" + std::to_string(x) + "/" +
                                     std::to_string(y) + ".png");
        request.SetTimeout(30);
        std::string png;
        if (!request.RunHttpRequest(png) || !WriteFile(path, png))
          return false;
      }
    }
  }
  return true;
}
}  // namespace grove
