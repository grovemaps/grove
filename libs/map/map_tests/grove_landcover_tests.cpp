#include "testing/testing.hpp"

#include "map/grove_landcover.hpp"

#include <cstring>
#include <string>
#include <vector>

namespace grove_landcover_tests
{
using namespace grove::landcover;

UNIT_TEST(GroveLandcover_SquareName)
{
  TEST_EQUAL(SquareName(51, 3), "N51E003", ());
  TEST_EQUAL(SquareName(-3, -63), "S03W063", ());
  TEST_EQUAL(SquareName(0, -180), "N00W180", ());
  TEST_EQUAL(SquareName(-90, 177), "S90E177", ());
}

UNIT_TEST(GroveLandcover_Colors)
{
  // Forests greener than fields, water and missing data left to the map.
  auto const trees = ClassColor(kTrees, false);
  auto const crops = ClassColor(kCrops, false);
  TEST_EQUAL(trees[3], 0xFF, ());
  TEST_LESS(trees[0], crops[0], ());
  TEST_EQUAL(ClassColor(kWater, false)[3], 0, ());
  TEST_EQUAL(ClassColor(kNoData, true)[3], 0, ());
  TEST_EQUAL(ClassColor(42, false)[3], 0, ());
  // Dark style colours are dark.
  TEST_LESS(ClassColor(kSnow, true)[0], 0x40, ());
}

namespace
{
void Put16(std::string & s, size_t at, uint16_t v)
{
  std::memcpy(s.data() + at, &v, 2);
}

void Put32(std::string & s, size_t at, uint32_t v)
{
  std::memcpy(s.data() + at, &v, 4);
}

// An IFD entry: tag, type (3 short, 4 long), count, value or offset.
void PutEntry(std::string & s, size_t at, uint16_t tag, uint16_t type, uint32_t count, uint32_t value)
{
  Put16(s, at, tag);
  Put16(s, at + 2, type);
  Put32(s, at + 4, count);
  if (type == 3 && count == 1)
    Put16(s, at + 8, static_cast<uint16_t>(value));
  else
    Put32(s, at + 8, value);
}
}  // namespace

UNIT_TEST(GroveLandcover_ParseLevels)
{
  // Two levels shaped like WorldCover's: 2048 px in 2x2 tiles of 1024, then a 562 px overview in one tile. The first
  // level's tile sizes lie beyond the head, as the full resolution arrays of real files may.
  std::string head(256, '\0');
  head.replace(0, 4, std::string("II*\0", 4));
  Put32(head, 4, 8);

  size_t const ifd0 = 8;
  Put16(head, ifd0, 7);
  PutEntry(head, ifd0 + 2, 256, 3, 1, 2048);
  PutEntry(head, ifd0 + 14, 257, 3, 1, 2048);
  PutEntry(head, ifd0 + 26, 259, 3, 1, 8);
  PutEntry(head, ifd0 + 38, 322, 3, 1, 1024);
  PutEntry(head, ifd0 + 50, 323, 3, 1, 1024);
  PutEntry(head, ifd0 + 62, 324, 4, 4, 200);   // Offsets, in the head.
  PutEntry(head, ifd0 + 74, 325, 4, 4, 5000);  // Sizes, beyond it.
  size_t const ifd1 = 100;
  Put32(head, ifd0 + 86, ifd1);
  for (uint32_t i = 0; i < 4; ++i)
    Put32(head, 200 + i * 4, 1000 + i);

  Put16(head, ifd1, 7);
  PutEntry(head, ifd1 + 2, 256, 3, 1, 562);
  PutEntry(head, ifd1 + 14, 257, 3, 1, 562);
  PutEntry(head, ifd1 + 26, 259, 3, 1, 8);
  PutEntry(head, ifd1 + 38, 322, 3, 1, 1024);
  PutEntry(head, ifd1 + 50, 323, 3, 1, 1024);
  PutEntry(head, ifd1 + 62, 324, 4, 1, 7777);
  PutEntry(head, ifd1 + 74, 325, 4, 1, 55);
  Put32(head, ifd1 + 86, 0);

  std::vector<std::pair<uint32_t, uint32_t>> reads;
  auto const levels = ParseLevels(head, [&reads](uint32_t offset, uint32_t size)
  {
    reads.emplace_back(offset, size);
    std::string more(size, '\0');
    for (uint32_t i = 0; i < size / 4; ++i)
      Put32(more, i * 4, 300 + i);
    return more;
  });

  TEST(levels, ());
  TEST_EQUAL(levels->size(), 2, ());
  auto const & full = (*levels)[0];
  TEST_EQUAL(full.m_width, 2048, ());
  TEST_EQUAL(full.m_tileWidth, 1024, ());
  TEST_EQUAL(full.m_offsets, std::vector<uint32_t>({1000, 1001, 1002, 1003}), ());
  TEST_EQUAL(full.m_sizes, std::vector<uint32_t>({300, 301, 302, 303}), ());
  TEST_EQUAL(reads, (std::vector<std::pair<uint32_t, uint32_t>>{{5000, 16}}), ());

  auto const & overview = (*levels)[1];
  TEST_EQUAL(overview.m_width, 562, ());
  TEST_EQUAL(overview.m_offsets, std::vector<uint32_t>({7777}), ());
  TEST_EQUAL(overview.m_sizes, std::vector<uint32_t>({55}), ());

  // Not a TIFF, or not deflated.
  TEST(!ParseLevels("<Error>NoSuchKey</Error>", nullptr), ());
  Put16(head, ifd1 + 26 + 8, 5);
  TEST(!ParseLevels(head, [](uint32_t, uint32_t) { return std::string(); }), ());
}
}  // namespace grove_landcover_tests
