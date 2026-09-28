#include "testing/testing.hpp"

#include "map/grove_relief.hpp"

#include <cmath>
#include <cstdint>
#include <functional>
#include <vector>

namespace grove_relief_tests
{
uint32_t constexpr kSize = 8;

// A Terrarium tile (rows south to north) with the given elevation at each pixel.
std::vector<uint8_t> MakeTile(std::function<double(int x, int y)> const & elevation)
{
  std::vector<uint8_t> rgba(kSize * kSize * 4);
  for (int y = 0; y < static_cast<int>(kSize); ++y)
  {
    for (int x = 0; x < static_cast<int>(kSize); ++x)
    {
      double const v = elevation(x, y) + 32768.0;
      uint8_t * px = &rgba[(y * kSize + x) * 4];
      px[0] = static_cast<uint8_t>(std::floor(v / 256));
      px[1] = static_cast<uint8_t>(static_cast<int>(std::floor(v)) % 256);
      px[2] = static_cast<uint8_t>((v - std::floor(v)) * 256);
      px[3] = 255;
    }
  }
  return rgba;
}

// Shade of the tile's middle pixel: > 0 lighter, < 0 darker, 0 unchanged.
int ShadeAtCenter(std::function<double(int x, int y)> const & elevation)
{
  auto rgba = MakeTile(elevation);
  grove::ShadeRelief(rgba, kSize, kSize, 30.0 /* metersPerPixel */, 1.0 /* exaggeration */);
  uint8_t const * px = &rgba[(kSize / 2 * kSize + kSize / 2) * 4];
  return px[0] > 128 ? px[3] : -px[3];
}

UNIT_TEST(GroveRelief_FlatGroundIsClear)
{
  TEST_EQUAL(ShadeAtCenter([](int, int) { return 120.0; }), 0, ());
  // Seabed relief is ignored.
  TEST_EQUAL(ShadeAtCenter([](int x, int) { return -100.0 - 50 * x; }), 0, ());
}

UNIT_TEST(GroveRelief_ElevationTint)
{
  TEST_EQUAL(grove::ElevationTint(100)[3], 0.0, ());
  auto const hills = grove::ElevationTint(1500);
  TEST_GREATER(hills[3], 0.1, ());
  // Warm: more red than blue.
  TEST_GREATER(hills[0], hills[2] + 40, ());
  // High mountains are paler than hills, and the tint never hides the map.
  TEST_GREATER(grove::ElevationTint(4000)[2], hills[2], ());
  for (double m : {500.0, 2000.0, 3000.0, 9000.0})
    TEST_LESS_OR_EQUAL(grove::ElevationTint(m)[3], 0.2, (m));

  // A flat plateau is tinted, but not shaded.
  auto rgba = MakeTile([](int, int) { return 1500.0; });
  grove::ShadeRelief(rgba, kSize, kSize, 30.0, 1.0);
  uint8_t const * px = &rgba[(kSize / 2 * kSize + kSize / 2) * 4];
  TEST_EQUAL(px[0], static_cast<uint8_t>(std::lround(hills[0])), ());
  TEST_EQUAL(px[3], static_cast<uint8_t>(std::lround(hills[3] * 255)), ());
}

UNIT_TEST(GroveRelief_SunFromNorthwest)
{
  // Ground rising to the east faces west, toward the sun; rising to the west faces away from it.
  TEST_GREATER(ShadeAtCenter([](int x, int) { return 250.0 + 10 * x; }), 0, ());
  TEST_LESS(ShadeAtCenter([](int x, int) { return 250.0 - 10 * x; }), 0, ());
  // Ground rising to the south faces north, toward the sun; rising to the north faces away from it.
  TEST_GREATER(ShadeAtCenter([](int, int y) { return 250.0 - 10 * y; }), 0, ());
  TEST_LESS(ShadeAtCenter([](int, int y) { return 250.0 + 10 * y; }), 0, ());
  // Steeper is darker.
  TEST_LESS(ShadeAtCenter([](int x, int) { return 250.0 - 30 * x; }),
            ShadeAtCenter([](int x, int) { return 250.0 - 10 * x; }), ());
}
}  // namespace grove_relief_tests
