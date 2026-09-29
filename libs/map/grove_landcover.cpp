#include "map/grove_landcover.hpp"

#include "map/grove_files.hpp"

#include "drape_frontend/grove_raster_layers.hpp"

#include "platform/grove_performance.hpp"
#include "platform/http_client.hpp"
#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/reader.hpp"
#include "coding/zlib.hpp"

#include "geometry/mercator.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"
#include "base/thread_pool_delayed.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <mutex>
#include <set>
#include <tuple>
#include <unordered_map>

#include "3party/stb_image/stb_image.h"

namespace grove::landcover
{
namespace
{
std::string_view constexpr kEnabledKey = "GroveLandcover";
std::string_view constexpr kUrl = "https://esa-worldcover.s3.eu-central-1.amazonaws.com/v200/2021/map/";

// From this zoom tiles come from WorldCover's files online (about 16 files for a zoom 6 tile); further out a tile would
// need dozens of them, so it comes from the bundled world pack (tools/grove/landcover_world.py).
int constexpr kMinOnlineZoom = 6;
// Up to this zoom, the world pack's tile shows while the online one loads, and offline where WorldCover's files aren't
// saved; closer in its 2.8 km pixels are too coarse.
int constexpr kMaxProvisionalZoom = 8;
std::string_view constexpr kWorldPack = "grove_landcover_world.bin";
std::string_view constexpr kWorldIndex = "grove_landcover_world.txt";
int constexpr kMaxWorldZoom = 5;  // Web mercator.

int constexpr kSquareDegrees = 3;
// Pixels per degree of the full resolution level; each overview halves it.
double constexpr kFullPixelsPerDegree = 12000;
uint32_t constexpr kTileSize = 256;  // World pack tiles.
// Map tiles: twice the usual 256 px, since phones show a tile 2-3 times larger than its pixels.
uint32_t constexpr kOutSize = 512;
// The headers of WorldCover's files, all levels' tile arrays included, fit in the first 28 KB.
uint32_t constexpr kHeadSize = 32 * 1024;
// Decoded overview tiles kept in memory: 1 MB each.
size_t constexpr kMaxDecodedTiles = 48;

std::string CacheDir()
{
  return base::JoinPath(GetPlatform().WritableDir(), "grove_landcover");
}

// Bytes of a WorldCover file, or nothing if it doesn't exist (open sea) or the network fails (then also error).
std::optional<std::string> ReadRange(std::string const & file, uint32_t offset, uint32_t size, bool & error)
{
  platform::HttpClient request(std::string(kUrl) + "ESA_WorldCover_10m_2021_v200_" + file + "_Map.tif");
  request.SetRawHeader("Range", "bytes=" + strings::to_string(offset) + "-" + strings::to_string(offset + size - 1));
  request.SetTimeout(30);
  error = false;
  if (request.RunHttpRequest() && (request.ErrorCode() == 206 || request.ErrorCode() == 200))
    return request.ServerResponse().substr(0, size);
  error = request.ErrorCode() != 404 && request.ErrorCode() != 403;
  return {};
}

std::optional<std::string> ReadCacheFile(std::string const & path)
{
  if (!Platform::IsFileExistsByFullPath(path))
    return {};
  try
  {
    std::string data;
    FileReader(path).ReadAsString(data);
    return data;
  }
  catch (RootException const &)
  {
    return {};
  }
}

void WriteCacheFile(std::string const & path, std::string const & data)
{
  UNUSED_VALUE(WriteAtomically(path, data));
}

// The coarsest level of a file that still has a pixel for each of a map tile's at the zoom. OM zoom Z is
// web-mercator zoom Z - 1.
size_t LevelFor(int zoom)
{
  double const needed = kOutSize * double(1 << (zoom - 1)) / 360;
  size_t levelIndex = 0;
  while (levelIndex < 6 && kFullPixelsPerDegree / (2 << levelIndex) >= needed)
    ++levelIndex;
  return levelIndex;
}

// The longitude of each sample column and the latitude of each sample row of a map tile, samples x samples a pixel.
// Rows run south to north. Panned around the world, the map shows copies of it east and west: their longitudes wrap
// back into [-180, 180).
std::pair<std::vector<double>, std::vector<double>> SampleCoordinates(m2::RectD const & rect, int samples)
{
  std::vector<double> lons(size_t{kOutSize} * samples), lats(size_t{kOutSize} * samples);
  for (size_t i = 0; i < lons.size(); ++i)
  {
    double const f = (i + 0.5) / lons.size();
    lons[i] = std::remainder(mercator::XToLon(rect.minX() + rect.SizeX() * f), 360.0);
    if (lons[i] >= 180)
      lons[i] -= 360;
    lats[i] = mercator::YToLat(rect.minY() + rect.SizeY() * f);
  }
  return {std::move(lons), std::move(lats)};
}

// Blurs a map tile's classes into soft blends (a box blur of the given radius, weighted by alpha, run across and
// down), so neighbouring classes shade into each other instead of showing as pixels.
void Soften(std::vector<uint8_t> & rgba, int radius)
{
  int constexpr n = kOutSize;
  std::vector<uint32_t> line(size_t{n} * 4);
  for (int pass = 0; pass < 2; ++pass)
  {
    for (int i = 0; i < n; ++i)
    {
      // Pass 0 blurs row i, pass 1 column i.
      auto const at = [&](int j) -> uint8_t *
      { return &rgba[(pass == 0 ? size_t(i) * n + j : size_t(j) * n + i) * 4]; };
      for (int j = 0; j < n; ++j)
      {
        uint32_t sum[4] = {0, 0, 0, 0};
        for (int k = j - radius; k <= j + radius; ++k)
        {
          uint8_t const * p = at(std::clamp(k, 0, n - 1));
          for (int c = 0; c < 3; ++c)
            sum[c] += p[c] * p[3];
          sum[3] += p[3];
        }
        uint32_t * out = &line[size_t(j) * 4];
        for (int c = 0; c < 3; ++c)
          out[c] = sum[3] ? sum[c] / sum[3] : 0;
        out[3] = sum[3] / (2 * radius + 1);
      }
      for (int j = 0; j < n; ++j)
        for (int c = 0; c < 4; ++c)
          at(j)[c] = static_cast<uint8_t>(line[size_t(j) * 4 + c]);
    }
  }
}

// WorldCover's files, their overview tiles and the map tiles being made from them.
class Provider
{
public:
  Provider(std::function<ref_ptr<df::DrapeEngine>()> getEngine, std::function<bool()> isDarkStyle)
    : m_getEngine(std::move(getEngine))
    , m_isDarkStyle(std::move(isDarkStyle))
  {
    UNUSED_VALUE(Platform::MkDirChecked(CacheDir()));
  }

  // Reads the files' tiles a region's map tiles need at the online zooms into the disk cache. Blocks; false on
  // network errors.
  bool Prefetch(m2::RectD const & mercatorRect)
  {
    auto const ll = mercator::ToLatLon(mercatorRect);
    for (int zoom = kMinOnlineZoom; zoom <= kLandcoverMaxZoom; ++zoom)
    {
      size_t const levelIndex = LevelFor(zoom);
      double const pixelsPerDegree = kFullPixelsPerDegree / (1 << levelIndex);
      for (int lat = static_cast<int>(std::floor(ll.minY() / kSquareDegrees)) * kSquareDegrees; lat < ll.maxY();
           lat += kSquareDegrees)
      {
        for (int lon = static_cast<int>(std::floor(ll.minX() / kSquareDegrees)) * kSquareDegrees; lon < ll.maxX();
             lon += kSquareDegrees)
        {
          std::string const square = SquareName(lat, lon);
          auto const levels = GetLevels(square);
          if (!levels)
            return false;
          if (levelIndex >= levels->size())
            continue;
          auto const & level = (*levels)[levelIndex];
          auto const pixel = [&](double degrees)
          { return static_cast<uint32_t>(std::max(0.0, degrees) * pixelsPerDegree); };
          uint32_t const x0 = pixel(ll.minX() - lon), x1 = std::min(pixel(ll.maxX() - lon), level.m_width - 1);
          uint32_t const y0 = pixel(lat + kSquareDegrees - ll.maxY());
          uint32_t const y1 = std::min(pixel(lat + kSquareDegrees - ll.minY()), level.m_height - 1);
          uint32_t const tilesAcross = (level.m_width + level.m_tileWidth - 1) / level.m_tileWidth;
          for (uint32_t ty = y0 / level.m_tileHeight; ty <= y1 / level.m_tileHeight; ++ty)
            for (uint32_t tx = x0 / level.m_tileWidth; tx <= x1 / level.m_tileWidth; ++tx)
              if (!GetTile(square, levelIndex, ty * tilesAcross + tx))
                return false;
        }
      }
    }
    return true;
  }

  bool RequestTile(df::TileKey const & tileKey, dp::BackgroundMode mode)
  {
    if (tileKey.m_zoomLevel < 1 || tileKey.m_zoomLevel > kLandcoverMaxZoom)
      return false;
    {
      std::lock_guard lock(m_mutex);
      m_active.insert(tileKey);
    }
    bool const dark = m_isDarkStyle();
    // The world pack's tiles take milliseconds: their own thread, so they never wait behind downloads.
    if (tileKey.m_zoomLevel <= kMaxProvisionalZoom)
    {
      m_packWorker.Push([this, tileKey, mode, dark]
      {
        if (IsActive(tileKey))
          MakeWorldTile(tileKey, mode, dark, tileKey.m_zoomLevel >= kMinOnlineZoom /* provisional */);
      });
    }
    if (tileKey.m_zoomLevel >= kMinOnlineZoom)
      m_workers.Push([this, tileKey, mode, dark] { MakeTile(tileKey, mode, dark); });
    return true;
  }

  void CancelTile(df::TileKey const & tileKey, dp::BackgroundMode)
  {
    std::lock_guard lock(m_mutex);
    m_active.erase(tileKey);
  }

private:
  using Pixels = std::shared_ptr<std::vector<uint8_t> const>;

  bool IsActive(df::TileKey const & tileKey)
  {
    std::lock_guard lock(m_mutex);
    return m_active.contains(tileKey);
  }

  // The file's levels; empty for squares without a file (sea). Nothing on network errors.
  std::optional<std::vector<Level>> GetLevels(std::string const & square)
  {
    {
      std::lock_guard lock(m_mutex);
      if (auto const it = m_levels.find(square); it != m_levels.end())
        return it->second;
    }

    std::string const path = base::JoinPath(CacheDir(), square + ".head");
    auto head = ReadCacheFile(path);
    bool error = false;
    if (!head)
    {
      head = ReadRange(square, 0, kHeadSize, error);
      if (error)
        return {};
      // An empty file marks a square without data.
      WriteCacheFile(path, head.value_or(""));
    }

    std::vector<Level> levels;
    if (!head->empty())
    {
      auto parsed = ParseLevels(*head, [&square, &error](uint32_t offset, uint32_t size)
      { return ReadRange(square, offset, size, error).value_or(""); });
      if (error)
        return {};
      if (parsed)
        levels = std::move(*parsed);
    }

    std::lock_guard lock(m_mutex);
    m_levels[square] = levels;
    return levels;
  }

  // A decoded tile of a level, or null where there is no data. Nothing on network errors.
  std::optional<Pixels> GetTile(std::string const & square, size_t levelIndex, uint32_t tileIndex)
  {
    std::string const key = square + "_" + strings::to_string(levelIndex) + "_" + strings::to_string(tileIndex);
    {
      std::lock_guard lock(m_mutex);
      if (auto const it = m_tiles.find(key); it != m_tiles.end())
      {
        m_tileOrder.splice(m_tileOrder.begin(), m_tileOrder, it->second.second);
        return it->second.first;
      }
    }

    auto const levels = GetLevels(square);
    if (!levels)
      return {};
    Pixels pixels;
    if (levelIndex < levels->size() && tileIndex < (*levels)[levelIndex].m_offsets.size())
    {
      auto const & level = (*levels)[levelIndex];
      std::string const path = base::JoinPath(CacheDir(), key + ".tile");
      auto compressed = ReadCacheFile(path);
      if (!compressed)
      {
        bool error = false;
        compressed = ReadRange(square, level.m_offsets[tileIndex], level.m_sizes[tileIndex], error);
        if (!compressed)
          return {};
        WriteCacheFile(path, *compressed);
      }

      auto decoded = std::make_shared<std::vector<uint8_t>>();
      decoded->reserve(size_t{level.m_tileWidth} * level.m_tileHeight);
      coding::ZLib::Inflate inflate(coding::ZLib::Inflate::Format::ZLib);
      if (!inflate(*compressed, std::back_inserter(*decoded)) ||
          decoded->size() != size_t{level.m_tileWidth} * level.m_tileHeight)
      {
        LOG(LWARNING, ("Bad WorldCover tile", key));
        decoded.reset();
      }
      pixels = std::move(decoded);
    }

    std::lock_guard lock(m_mutex);
    m_tileOrder.push_front(key);
    m_tiles[key] = {pixels, m_tileOrder.begin()};
    while (m_tiles.size() > kMaxDecodedTiles)
    {
      m_tiles.erase(m_tileOrder.back());
      m_tileOrder.pop_back();
    }
    return pixels;
  }

  // The bundled world pack's tile, decoded: WorldCover classes. Empty for tiles without land.
  std::shared_ptr<std::vector<uint8_t> const> GetWorldTile(int z, int x, int y)
  {
    std::lock_guard lock(m_mutex);
    if (!m_worldLoaded)
    {
      m_worldLoaded = true;
      try
      {
        m_worldPack = GetPlatform().GetReader(std::string(kWorldPack));
        std::string index;
        GetPlatform().GetReader(std::string(kWorldIndex))->ReadAsString(index);
        strings::Tokenize(index, "\n", [this](std::string_view line)
        {
          if (line.empty() || line.front() == '#')
            return;
          std::vector<std::string_view> f;
          strings::Tokenize(line, "\t", [&f](std::string_view v) { f.push_back(v); });
          int tz, tx, ty;
          uint64_t offset, size;
          if (f.size() == 5 && strings::to_int(f[0], tz) && strings::to_int(f[1], tx) && strings::to_int(f[2], ty) &&
              strings::to_uint(f[3], offset) && strings::to_uint(f[4], size))
            m_worldIndex[{tz, tx, ty}] = {offset, size};
        });
      }
      catch (RootException const & e)
      {
        LOG(LWARNING, ("No world land cover:", e.Msg()));
      }
    }

    auto const key = std::tuple(z, x, y);
    if (auto const it = m_worldTiles.find(key); it != m_worldTiles.end())
      return it->second;
    std::shared_ptr<std::vector<uint8_t> const> tile;
    if (auto const it = m_worldIndex.find(key); it != m_worldIndex.end() && m_worldPack)
    {
      std::vector<uint8_t> png(it->second.second);
      m_worldPack->Read(it->second.first, png.data(), png.size());
      int w = 0, h = 0, comp = 0;
      if (stbi_uc * data = stbi_load_from_memory(png.data(), static_cast<int>(png.size()), &w, &h, &comp, 1))
      {
        if (w == int(kTileSize) && h == int(kTileSize))
          tile = std::make_shared<std::vector<uint8_t> const>(data, data + kTileSize * kTileSize);
        stbi_image_free(data);
      }
    }
    // A zoomed-out view needs a few dozen of them at most: keep them all.
    m_worldTiles[key] = tile;
    return tile;
  }

  void MakeTile(df::TileKey const & tileKey, dp::BackgroundMode mode, bool dark)
  {
    if (!IsActive(tileKey))
      return;

    size_t const levelIndex = LevelFor(tileKey.m_zoomLevel);
    double const pixelsPerDegree = kFullPixelsPerDegree / (1 << levelIndex);

    m2::RectD const rect = tileKey.GetGlobalRect();
    std::vector<uint8_t> rgba(size_t{kOutSize} * kOutSize * 4, 0);
    bool empty = true;
    // Each pixel averages 2x2 samples, which smooths the classes' edges. A sample's longitude depends only on its
    // column and its latitude only on its row: work them out once.
    int constexpr kSamples = 2;
    auto const [lons, lats] = SampleCoordinates(rect, kSamples);
    // The square and tile of the last sample: neighbours mostly share them.
    int cachedLat = std::numeric_limits<int>::max();
    int cachedLon = std::numeric_limits<int>::max();
    std::string square;
    std::optional<std::vector<Level>> levels;
    uint32_t cachedTile = std::numeric_limits<uint32_t>::max();
    Pixels tile;
    for (uint32_t row = 0; row < kOutSize; ++row)
    {
      for (uint32_t col = 0; col < kOutSize; ++col)
      {
        uint32_t sum[4] = {0, 0, 0, 0};
        for (int sy = 0; sy < kSamples; ++sy)
        {
          for (int sx = 0; sx < kSamples; ++sx)
          {
            // Texture rows run south to north, as the relief's.
            double const lon = lons[col * kSamples + sx];
            double const lat = lats[row * kSamples + sy];
            int const squareLat = static_cast<int>(std::floor(lat / kSquareDegrees)) * kSquareDegrees;
            int const squareLon = static_cast<int>(std::floor(lon / kSquareDegrees)) * kSquareDegrees;
            if (squareLat != cachedLat || squareLon != cachedLon)
            {
              cachedLat = squareLat;
              cachedLon = squareLon;
              square = SquareName(squareLat, squareLon);
              levels = GetLevels(square);
              if (!levels)
                return;  // Network error: the tile is requested again when it shows next time.
              cachedTile = std::numeric_limits<uint32_t>::max();
            }
            if (levelIndex >= levels->size())
              continue;
            auto const & level = (*levels)[levelIndex];
            auto const px = static_cast<uint32_t>((lon - squareLon) * pixelsPerDegree);
            auto const py = static_cast<uint32_t>((squareLat + kSquareDegrees - lat) * pixelsPerDegree);
            if (px >= level.m_width || py >= level.m_height)
              continue;
            uint32_t const tilesAcross = (level.m_width + level.m_tileWidth - 1) / level.m_tileWidth;
            uint32_t const tileIndex = (py / level.m_tileHeight) * tilesAcross + px / level.m_tileWidth;
            if (tileIndex != cachedTile)
            {
              auto fetched = GetTile(square, levelIndex, tileIndex);
              if (!fetched)
                return;
              tile = std::move(*fetched);
              cachedTile = tileIndex;
            }
            if (!tile)
              continue;
            uint8_t const cls = (*tile)[(py % level.m_tileHeight) * level.m_tileWidth + px % level.m_tileWidth];
            auto const color = ClassColor(cls, dark);
            for (int c = 0; c < 3; ++c)
              sum[c] += color[c] * color[3];
            sum[3] += color[3];
          }
        }
        if (sum[3] == 0)
          continue;
        empty = false;
        uint8_t * out = &rgba[(size_t{row} * kOutSize + col) * 4];
        for (int c = 0; c < 3; ++c)
          out[c] = static_cast<uint8_t>(sum[c] / sum[3]);
        out[3] = static_cast<uint8_t>(sum[3] / (kSamples * kSamples));
      }
      if (!IsActive(tileKey))
        return;
    }

    if (!empty)
    {
      Soften(rgba, 1);
      Deliver(tileKey, mode, dark, std::move(rgba));
    }
  }

  // Zoomed out: the bundled world pack, with 2x2 samples a pixel as online.
  void MakeWorldTile(df::TileKey const & tileKey, dp::BackgroundMode mode, bool dark, bool provisional = false)
  {
    // OM zoom Z is web mercator zoom Z - 1; a 512 px tile needs the pack's next zoom.
    int const z = std::min(int(tileKey.m_zoomLevel), kMaxWorldZoom);
    double const n = 1 << z;
    m2::RectD const rect = tileKey.GetGlobalRect();
    std::vector<uint8_t> rgba(size_t{kOutSize} * kOutSize * 4, 0);
    bool empty = true;
    int constexpr kSamples = 2;
    // Each sample's position in the pack's tile grid, by column and by row.
    auto const [lons, lats] = SampleCoordinates(rect, kSamples);
    std::vector<double> fxs(lons.size()), fys(lats.size());
    for (size_t i = 0; i < lons.size(); ++i)
      fxs[i] = (lons[i] + 180) / 360 * n;
    for (size_t i = 0; i < lats.size(); ++i)
      fys[i] = (1 - std::asinh(std::tan(lats[i] * M_PI / 180)) / M_PI) / 2 * n;

    int cachedX = -1, cachedY = -1;
    std::shared_ptr<std::vector<uint8_t> const> tile;
    for (uint32_t row = 0; row < kOutSize; ++row)
    {
      for (uint32_t col = 0; col < kOutSize; ++col)
      {
        uint32_t sum[4] = {0, 0, 0, 0};
        for (int sy = 0; sy < kSamples; ++sy)
        {
          for (int sx = 0; sx < kSamples; ++sx)
          {
            double const fx = fxs[col * kSamples + sx], fy = fys[row * kSamples + sy];
            if (fx < 0 || fx >= n)
              continue;
            int const tx = static_cast<int>(fx);
            int const ty = std::clamp(static_cast<int>(fy), 0, int(n) - 1);
            if (tx != cachedX || ty != cachedY)
            {
              tile = GetWorldTile(z, tx, ty);
              cachedX = tx;
              cachedY = ty;
            }
            if (!tile)
              continue;
            auto const px = std::clamp(static_cast<int>((fx - tx) * kTileSize), 0, int(kTileSize) - 1);
            auto const py = std::clamp(static_cast<int>((fy - ty) * kTileSize), 0, int(kTileSize) - 1);
            auto const color = ClassColor((*tile)[py * kTileSize + px], dark);
            for (int c = 0; c < 3; ++c)
              sum[c] += color[c] * color[3];
            sum[3] += color[3];
          }
        }
        if (sum[3] == 0)
          continue;
        empty = false;
        uint8_t * out = &rgba[(size_t{row} * kOutSize + col) * 4];
        for (int c = 0; c < 3; ++c)
          out[c] = static_cast<uint8_t>(sum[c] / sum[3]);
        out[3] = static_cast<uint8_t>(sum[3] / (kSamples * kSamples));
      }
    }
    if (!empty)
    {
      // The pack's 2.8 km pixels: blend classes into gradients rather than speckles.
      Soften(rgba, 3);
      Deliver(tileKey, mode, dark, std::move(rgba), provisional);
    }
  }

  // A provisional tile (from the world pack, while the online one loads) keeps the request open; its image has its own
  // uid, since the renderer ignores a second upload under the same one.
  void Deliver(df::TileKey const & tileKey, dp::BackgroundMode mode, bool dark, std::vector<uint8_t> && rgba,
               bool provisional = false)
  {
    auto engine = m_getEngine();
    if (!engine || !IsActive(tileKey))
      return;
    if (!provisional)
      CancelTile(tileKey, mode);
    std::string const uid = std::string(ImagePrefix(dp::BackgroundMode::Landcover)) + (dark ? "dark/" : "light/") +
                            strings::to_string(int(tileKey.m_zoomLevel)) + "/" + strings::to_string(tileKey.m_x) + "/" +
                            strings::to_string(tileKey.m_y) + (provisional ? "/pack" : "");
    engine->AddTileBackgroundImage(uid, kOutSize, kOutSize, dp::TextureFormat::RGBA8, mode, std::move(rgba));
    engine->SetTileBackgroundData(tileKey, uid, m2::RectF(0, 0, 1, 1));
  }

  std::function<ref_ptr<df::DrapeEngine>()> const m_getEngine;
  std::function<bool()> const m_isDarkStyle;
  // Its own threads, not the platform's one network thread: a tile waits for its downloads there. More with the
  // performance boost (platform/grove_performance.hpp). JVM-attached on Android, which HTTP requests need.
  base::DelayedThreadPool m_workers{PerformanceBoost() ? 4u : 2u, base::DelayedThreadPool::Exit::SkipPending};
  // The world pack's tiles: zoom 5 and out, and shown while the online ones load up to kMaxProvisionalZoom.
  base::DelayedThreadPool m_packWorker{1, base::DelayedThreadPool::Exit::SkipPending};

  std::mutex m_mutex;
  std::set<df::TileKey> m_active;
  std::unordered_map<std::string, std::vector<Level>> m_levels;
  std::list<std::string> m_tileOrder;  // Most recently used first.
  std::map<std::string, std::pair<Pixels, std::list<std::string>::iterator>> m_tiles;
  bool m_worldLoaded = false;
  std::unique_ptr<ModelReader> m_worldPack;
  std::map<std::tuple<int, int, int>, std::pair<uint64_t, uint64_t>> m_worldIndex;
  std::map<std::tuple<int, int, int>, std::shared_ptr<std::vector<uint8_t> const>> m_worldTiles;
};

uint32_t Read32(std::string const & s, size_t offset)
{
  uint32_t v;
  std::memcpy(&v, s.data() + offset, 4);
  return v;
}

uint16_t Read16(std::string const & s, size_t offset)
{
  uint16_t v;
  std::memcpy(&v, s.data() + offset, 2);
  return v;
}
}  // namespace

std::array<uint8_t, 4> ClassColor(uint8_t cls, bool dark)
{
  // Grove's light palette, at its zoom 11 shades (data/styles/grove/palette-light.mapcss); the dark ones follow
  // upstream's dark style.
  switch (cls)
  {
  case kTrees:
    return dark ? std::array<uint8_t, 4>{0x2B, 0x33, 0x16, 0xFF} : std::array<uint8_t, 4>{0xC2, 0xE5, 0xA8, 0xFF};
  case kShrubs:
    return dark ? std::array<uint8_t, 4>{0x2A, 0x30, 0x17, 0xFF} : std::array<uint8_t, 4>{0xCD, 0xE8, 0xAE, 0xFF};
  case kGrass:
    return dark ? std::array<uint8_t, 4>{0x26, 0x2C, 0x16, 0xFF} : std::array<uint8_t, 4>{0xDD, 0xEE, 0xBD, 0xFF};
  case kCrops:
    return dark ? std::array<uint8_t, 4>{0x24, 0x22, 0x10, 0xFF} : std::array<uint8_t, 4>{0xE8, 0xF1, 0xCF, 0xFF};
  case kBuilt:
    return dark ? std::array<uint8_t, 4>{0x24, 0x24, 0x24, 0xFF} : std::array<uint8_t, 4>{0xE6, 0xE6, 0xE4, 0xFF};
  case kBare:
    return dark ? std::array<uint8_t, 4>{0x2A, 0x25, 0x24, 0xFF} : std::array<uint8_t, 4>{0xEE, 0xEA, 0xDC, 0xFF};
  case kSnow:
    return dark ? std::array<uint8_t, 4>{0x1C, 0x1E, 0x20, 0xFF} : std::array<uint8_t, 4>{0xFA, 0xFC, 0xFD, 0xFF};
  case kWetland:
    return dark ? std::array<uint8_t, 4>{0x23, 0x30, 0x33, 0xFF} : std::array<uint8_t, 4>{0xD2, 0xE9, 0xD6, 0xFF};
  case kMangroves:
    return dark ? std::array<uint8_t, 4>{0x1E, 0x33, 0x22, 0xFF} : std::array<uint8_t, 4>{0xB5, 0xDF, 0xB0, 0xFF};
  case kMoss:
    return dark ? std::array<uint8_t, 4>{0x2A, 0x2F, 0x1C, 0xFF} : std::array<uint8_t, 4>{0xE4, 0xED, 0xBF, 0xFF};
  default: return {0, 0, 0, 0};
  }
}

std::string SquareName(int lat, int lon)
{
  auto const pad = [](int v, int width)
  {
    std::string s = strings::to_string(std::abs(v));
    return std::string(width - std::min<int>(width, s.size()), '0') + s;
  };
  return std::string(lat < 0 ? "S" : "N") + pad(lat, 2) + (lon < 0 ? "W" : "E") + pad(lon, 3);
}

std::optional<std::vector<Level>> ParseLevels(std::string const & head,
                                              std::function<std::string(uint32_t, uint32_t)> const & readMore)
{
  if (head.size() < 8 || head.compare(0, 4, std::string("II*\0", 4)) != 0)
    return {};

  // An array of 32-bit values, from the head or read beyond it.
  auto const readArray = [&](uint16_t type, uint32_t count, uint32_t valueOrOffset) -> std::vector<uint32_t>
  {
    size_t const itemSize = type == 3 ? 2 : 4;
    std::string inlineBytes(4, '\0');
    std::memcpy(inlineBytes.data(), &valueOrOffset, 4);
    std::string const * bytes = &inlineBytes;
    std::string more;
    size_t base = 0;
    if (count * itemSize > 4)
    {
      if (valueOrOffset + count * itemSize <= head.size())
      {
        bytes = &head;
        base = valueOrOffset;
      }
      else
      {
        more = readMore(valueOrOffset, count * itemSize);
        if (more.size() < count * itemSize)
          return {};
        bytes = &more;
      }
    }
    std::vector<uint32_t> values(count);
    for (uint32_t i = 0; i < count; ++i)
      values[i] = itemSize == 2 ? Read16(*bytes, base + i * 2) : Read32(*bytes, base + i * 4);
    return values;
  };

  std::vector<Level> levels;
  uint32_t ifd = Read32(head, 4);
  while (ifd != 0 && ifd + 2 <= head.size())
  {
    uint16_t const count = Read16(head, ifd);
    if (ifd + 2 + count * 12 + 4 > head.size())
      return {};
    Level level;
    for (uint16_t i = 0; i < count; ++i)
    {
      size_t const entry = ifd + 2 + i * 12;
      uint16_t const tag = Read16(head, entry);
      uint16_t const type = Read16(head, entry + 2);
      uint32_t const n = Read32(head, entry + 4);
      uint32_t const value = Read32(head, entry + 8);
      uint32_t const scalar = type == 3 ? Read16(head, entry + 8) : value;
      switch (tag)
      {
      case 256: level.m_width = scalar; break;
      case 257: level.m_height = scalar; break;
      case 259:
        if (scalar != 8)  // Deflate.
          return {};
        break;
      case 322: level.m_tileWidth = scalar; break;
      case 323: level.m_tileHeight = scalar; break;
      case 324: level.m_offsets = readArray(type, n, value); break;
      case 325: level.m_sizes = readArray(type, n, value); break;
      default: break;
      }
    }
    if (level.m_tileWidth == 0 || level.m_tileHeight == 0 || level.m_offsets.empty() ||
        level.m_offsets.size() != level.m_sizes.size())
      return {};
    levels.push_back(std::move(level));
    ifd = Read32(head, ifd + 2 + count * 12);
  }
  if (levels.empty())
    return {};
  return levels;
}

namespace
{
Provider * g_provider = nullptr;
}  // namespace

void CreateProvider(std::function<ref_ptr<df::DrapeEngine>()> getEngine, std::function<bool()> isDarkStyle)
{
  RasterLayerEnabled(dp::BackgroundMode::Landcover) = IsEnabled();
  // Never destroyed: its tasks may still run on the network threads when the app closes.
  if (!g_provider)
    g_provider = new Provider(std::move(getEngine), std::move(isDarkStyle));
  GetRasterLayerSource(dp::BackgroundMode::Landcover) = {[](df::TileKey const & key, dp::BackgroundMode mode)
  { return g_provider->RequestTile(key, mode); }, [](df::TileKey const & key, dp::BackgroundMode mode)
  { g_provider->CancelTile(key, mode); }};
}

bool Prefetch(m2::RectD const & mercatorRect)
{
  return !g_provider || g_provider->Prefetch(mercatorRect);
}

bool IsEnabled()
{
  bool enabled = true;
  settings::TryGet(kEnabledKey, enabled);
  return enabled;
}

void SetEnabled(ref_ptr<df::DrapeEngine> engine, bool enabled)
{
  settings::Set(kEnabledKey, enabled);
  RasterLayerEnabled(dp::BackgroundMode::Landcover) = enabled;
  // Re-reads the visible tiles, which also updates the layer's viewport (requests or drops its tiles).
  if (engine)
    engine->InvalidateRect(mercator::Bounds::FullRect());
}
}  // namespace grove::landcover
