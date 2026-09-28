#include "map/grove_brand_places.hpp"

#include "drape_frontend/grove_brands.hpp"

#include "drape/grove_brand_texture.hpp"

#include "indexer/feature.hpp"
#include "indexer/scales.hpp"

#include "platform/platform.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/timer.hpp"

#include <algorithm>
#include <bit>

namespace grove
{
namespace
{
uint32_t constexpr kCacheFormat = 1;
}  // namespace

BrandPlaces::BrandPlaces(DataSource const & dataSource, std::string cacheDir,
                         std::function<void(m2::RectD const &)> onReady)
  : m_dataSource(dataSource)
  , m_cacheDir(std::move(cacheDir))
  , m_onReady(std::move(onReady))
  , m_worker(&BrandPlaces::Work, this)
{}

BrandPlaces::~BrandPlaces()
{
  {
    std::lock_guard lock(m_mutex);
    m_stop = true;
  }
  m_wake.notify_one();
  m_worker.join();
}

void BrandPlaces::ForEachInRect(MwmSet::MwmId const & mwm, m2::RectD const & rect, std::vector<BrandPlace> & places)
{
  if (!mwm.IsAlive() || mwm.GetInfo()->GetType() != MwmInfo::COUNTRY)
    return;

  std::lock_guard lock(m_mutex);
  auto const [it, isNew] = m_places.try_emplace(mwm);
  if (isNew)
  {
    m_queue.push_back(mwm);
    m_wake.notify_one();
    return;
  }

  auto const & entries = it->second;
  auto e = std::ranges::lower_bound(entries, rect.minX(), {}, [](Entry const & e) { return e.m_point.x; });
  for (; e != entries.end() && e->m_point.x <= rect.maxX(); ++e)
    if (rect.IsPointInside(e->m_point))
      places.push_back({FeatureID(mwm, e->m_index), e->m_point, e->m_brand});
}

void BrandPlaces::Work()
{
  while (true)
  {
    MwmSet::MwmId mwm;
    {
      std::unique_lock lock(m_mutex);
      m_wake.wait(lock, [this] { return m_stop || !m_queue.empty(); });
      if (m_stop)
        return;
      mwm = m_queue.front();
      m_queue.pop_front();
    }

    // A map without chains keeps its empty list.
    auto entries = Load(mwm);
    if (entries.empty())
      continue;

    {
      std::lock_guard lock(m_mutex);
      m_places[mwm] = std::move(entries);
    }
    m_onReady(mwm.GetInfo()->m_bordersRect);
  }
}

std::string BrandPlaces::CachePath(MwmSet::MwmId const & mwm) const
{
  return base::JoinPath(m_cacheDir, mwm.GetInfo()->GetCountryName() + ".bin");
}

std::vector<BrandPlaces::Entry> BrandPlaces::Load(MwmSet::MwmId const & mwm) const
{
  auto const version = static_cast<uint64_t>(mwm.GetInfo()->GetVersion());
  uint64_t const signature = BrandPack::Instance().GetSignature();
  std::string const path = CachePath(mwm);

  std::vector<Entry> entries;
  if (Platform::IsFileExistsByFullPath(path))
  {
    try
    {
      FileReader reader(path);
      ReaderSource<FileReader> src(reader);
      if (ReadPrimitiveFromSource<uint32_t>(src) == kCacheFormat && ReadPrimitiveFromSource<uint64_t>(src) == version &&
          ReadPrimitiveFromSource<uint64_t>(src) == signature)
      {
        entries.resize(ReadPrimitiveFromSource<uint32_t>(src));
        for (auto & e : entries)
        {
          e.m_index = ReadPrimitiveFromSource<uint32_t>(src);
          e.m_point.x = std::bit_cast<double>(ReadPrimitiveFromSource<uint64_t>(src));
          e.m_point.y = std::bit_cast<double>(ReadPrimitiveFromSource<uint64_t>(src));
          rw::Read(src, e.m_brand);
        }
        return entries;
      }
    }
    catch (RootException const & e)
    {
      LOG(LWARNING, ("Can't read", path, e.Msg()));
    }
  }

  base::Timer timer;
  bool stopped = false;
  entries = Collect(m_dataSource, mwm, [this, &stopped]
  {
    std::lock_guard lock(m_mutex);
    return stopped = m_stop;
  });
  if (stopped || !mwm.IsAlive())
    return {};
  LOG(LINFO,
      ("Chains' places of", mwm.GetInfo()->GetCountryName(), ":", entries.size(), "in", timer.ElapsedSeconds(), "s"));

  try
  {
    UNUSED_VALUE(Platform::MkDirChecked(m_cacheDir));
    FileWriter writer(path);
    WriteToSink(writer, kCacheFormat);
    WriteToSink(writer, version);
    WriteToSink(writer, signature);
    WriteToSink(writer, static_cast<uint32_t>(entries.size()));
    for (auto const & e : entries)
    {
      WriteToSink(writer, e.m_index);
      WriteToSink(writer, std::bit_cast<uint64_t>(e.m_point.x));
      WriteToSink(writer, std::bit_cast<uint64_t>(e.m_point.y));
      rw::Write(writer, e.m_brand);
    }
  }
  catch (RootException const & e)
  {
    LOG(LWARNING, ("Can't write", path, e.Msg()));
  }
  return entries;
}

std::vector<BrandPlaces::Entry> BrandPlaces::Collect(DataSource const & dataSource, MwmSet::MwmId const & mwm,
                                                     std::function<bool()> const & stop)
{
  std::vector<Entry> entries;
  FeaturesLoaderGuard guard(dataSource, mwm);
  size_t const count = guard.GetNumFeatures();
  for (uint32_t i = 0; i < count; ++i)
  {
    if (i % 4096 == 0 && stop())
      return {};

    auto ft = guard.GetFeatureByIndex(i);
    if (!ft || ft->GetGeomType() == feature::GeomType::Line)
      continue;
    auto const brand = FindBrand(*ft);
    if (brand.empty())
      continue;

    m2::PointD const point = ft->GetGeomType() == feature::GeomType::Point
                               ? ft->GetCenter()
                               : ft->GetLimitRect(scales::GetUpperScale()).Center();
    entries.push_back({i, point, std::string(brand)});
  }
  std::ranges::sort(entries, {}, [](Entry const & e) { return e.m_point.x; });
  return entries;
}
}  // namespace grove
