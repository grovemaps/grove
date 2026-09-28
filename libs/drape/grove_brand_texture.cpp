#include "drape/grove_brand_texture.hpp"

#include "platform/platform.hpp"

#include "coding/reader_streambuf.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "3party/stb_image/stb_image.h"

#include <algorithm>
#include <cmath>
#include <istream>

namespace grove
{
namespace
{
std::string_view constexpr kIndexFile = "grove_brands.txt";
std::string_view constexpr kPackFile = "grove_brands.bin";
uint32_t constexpr kTextureSize = 1024;

std::string Normalize(std::string_view name)
{
  std::string s(name);
  strings::MakeLowerCaseInplace(s);
  return s;
}

// Area-averaging downscale of a square RGBA image, which keeps thin logo strokes smooth.
std::vector<uint8_t> Downscale(uint8_t const * src, uint32_t srcSize, uint32_t dstSize)
{
  std::vector<uint8_t> dst(size_t{dstSize} * dstSize * 4);
  double const ratio = static_cast<double>(srcSize) / dstSize;
  for (uint32_t y = 0; y < dstSize; ++y)
  {
    for (uint32_t x = 0; x < dstSize; ++x)
    {
      auto const x0 = static_cast<uint32_t>(x * ratio);
      auto const y0 = static_cast<uint32_t>(y * ratio);
      auto const x1 = std::max(x0 + 1, static_cast<uint32_t>(std::ceil((x + 1) * ratio)));
      auto const y1 = std::max(y0 + 1, static_cast<uint32_t>(std::ceil((y + 1) * ratio)));
      // Premultiplied sums, so transparent pixels don't darken the edges.
      double sum[4] = {};
      for (uint32_t sy = y0; sy < std::min(y1, srcSize); ++sy)
      {
        for (uint32_t sx = x0; sx < std::min(x1, srcSize); ++sx)
        {
          uint8_t const * p = src + (size_t{sy} * srcSize + sx) * 4;
          double const a = p[3] / 255.0;
          sum[0] += p[0] * a;
          sum[1] += p[1] * a;
          sum[2] += p[2] * a;
          sum[3] += a;
        }
      }
      double const count = static_cast<double>(std::min(x1, srcSize) - x0) * (std::min(y1, srcSize) - y0);
      uint8_t * d = &dst[(size_t{y} * dstSize + x) * 4];
      for (int c = 0; c < 3; ++c)
        d[c] = sum[3] > 0 ? static_cast<uint8_t>(std::lround(sum[c] / sum[3])) : 0;
      d[3] = static_cast<uint8_t>(std::lround(255 * sum[3] / count));
    }
  }
  return dst;
}
}  // namespace

BrandPack const & BrandPack::Instance()
{
  static BrandPack const pack;
  return pack;
}

BrandPack::BrandPack()
{
  try
  {
    m_pack = GetPlatform().GetReader(std::string(kPackFile));
    ReaderStreamBuf buffer(GetPlatform().GetReader(std::string(kIndexFile)));
    std::istream in(&buffer);
    std::string line;
    while (std::getline(in, line))
    {
      if (line.empty() || line.front() == '#')
        continue;
      std::vector<std::string_view> fields;
      strings::Tokenize(line, "\t", [&fields](std::string_view f) { fields.push_back(f); });
      if (fields.size() != 6)
        continue;

      Brand brand;
      brand.m_qid = fields[0];
      if (!strings::to_uint(fields[1], brand.m_offset) || !strings::to_uint(fields[2], brand.m_size))
        continue;
      if (fields[3] != "*")
        strings::Tokenize(fields[3], ";", [&brand](std::string_view c) { brand.m_countries.emplace_back(c); });
      strings::Tokenize(fields[5], ";", [&brand](std::string_view t) { brand.m_types.emplace_back(t); });

      size_t const i = m_brands.size();
      m_byQid.emplace(brand.m_qid, i);
      strings::Tokenize(fields[4], "|", [this, i](std::string_view n) { m_byName[Normalize(n)].push_back(i); });
      m_brands.push_back(std::move(brand));
    }
  }
  catch (RootException const & e)
  {
    LOG(LWARNING, ("No brand logos:", e.Msg()));
  }
}

bool BrandPack::HasName(std::string_view name) const
{
  return m_byName.contains(Normalize(name));
}

std::string_view BrandPack::Find(std::string_view name, std::string_view country,
                                 std::vector<std::string> const & placeTypes) const
{
  auto const it = m_byName.find(Normalize(name));
  if (it == m_byName.end())
    return {};

  // A brand of this country wins over a worldwide one; the most used goes first within each.
  std::string_view anywhere;
  for (size_t const i : it->second)
  {
    auto const & brand = m_brands[i];
    if (std::ranges::none_of(placeTypes, [&brand](std::string const & t)
    { return std::ranges::find(brand.m_types, t) != brand.m_types.end(); }))
    {
      continue;
    }
    if (brand.m_countries.empty())
    {
      if (anywhere.empty())
        anywhere = brand.m_qid;
    }
    else if (std::find(brand.m_countries.begin(), brand.m_countries.end(), country) != brand.m_countries.end())
    {
      return brand.m_qid;
    }
  }
  return anywhere;
}

std::vector<uint8_t> BrandPack::ReadBadge(std::string_view qid) const
{
  auto const it = m_byQid.find(std::string(qid));
  if (it == m_byQid.end() || !m_pack)
    return {};
  auto const & brand = m_brands[it->second];
  std::vector<uint8_t> png(brand.m_size);
  m_pack->Read(brand.m_offset, png.data(), png.size());
  return png;
}

BrandIndex::BrandIndex(m2::PointU const & textureSize, uint32_t slotSize)
  : m_textureSize(textureSize)
  , m_slotSize(slotSize)
{}

bool BrandIndex::Prepare(std::string const & qid)
{
  std::lock_guard lock(m_mutex);
  if (m_mapping.contains(qid))
    return true;
  if (m_failed.contains(qid))
    return false;

  uint32_t const columns = m_textureSize.x / m_slotSize;
  uint32_t const rows = m_textureSize.y / m_slotSize;
  if (m_nextSlot >= columns * rows)
    return false;  // Full: this session keeps category icons for further brands.

  auto const png = BrandPack::Instance().ReadBadge(qid);
  int w = 0, h = 0, comp = 0;
  stbi_uc * data =
      png.empty() ? nullptr : stbi_load_from_memory(png.data(), static_cast<int>(png.size()), &w, &h, &comp, 4);
  if (data == nullptr || w != h)
  {
    if (data)
      stbi_image_free(data);
    m_failed.insert(qid);
    return false;
  }
  auto pixels = Downscale(data, static_cast<uint32_t>(w), m_slotSize);
  stbi_image_free(data);

  uint32_t const x = (m_nextSlot % columns) * m_slotSize;
  uint32_t const y = (m_nextSlot / columns) * m_slotSize;
  ++m_nextSlot;

  m2::RectU const rect(x, y, x + m_slotSize, y + m_slotSize);
  m2::RectF const texRect(static_cast<float>(x) / m_textureSize.x, static_cast<float>(y) / m_textureSize.y,
                          static_cast<float>(x + m_slotSize) / m_textureSize.x,
                          static_cast<float>(y + m_slotSize) / m_textureSize.y);
  m_mapping.emplace(qid, BrandResourceInfo(texRect));
  m_pending.push_back({rect, std::move(pixels)});
  return true;
}

ref_ptr<dp::Texture::ResourceInfo> BrandIndex::MapResource(BrandKey const & key, bool & newResource)
{
  newResource = false;
  std::lock_guard lock(m_mutex);
  auto const it = m_mapping.find(key.m_qid);
  ASSERT(it != m_mapping.end(), ("Brand badge used before Prepare", key.m_qid));
  return it != m_mapping.end() ? make_ref(&it->second) : nullptr;
}

void BrandIndex::UploadResources(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Texture> texture)
{
  std::vector<Pending> pending;
  {
    std::lock_guard lock(m_mutex);
    pending.swap(m_pending);
  }
  for (auto & p : pending)
  {
    texture->UploadData(context, p.m_rect.minX(), p.m_rect.minY(), p.m_rect.SizeX(), p.m_rect.SizeY(),
                        make_ref(p.m_pixels.data()));
  }
}

BrandTexture::BrandTexture(ref_ptr<dp::HWTextureAllocator> allocator, double visualScale)
  : m_index({kTextureSize, kTextureSize}, static_cast<uint32_t>(std::lround(kBrandBadgeDp * visualScale)))
{
  Base::Init(allocator, make_ref(&m_index),
             {{kTextureSize, kTextureSize}, dp::TextureFormat::RGBA8, dp::TextureFilter::Nearest, false});
}
}  // namespace grove
