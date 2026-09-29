#include "testing/testing.hpp"

#include "modules/logos/drape/brand_texture.hpp"

#include <string>
#include <vector>

namespace grove_brand_tests
{
using grove::BrandPack;

// Uses the generated data/grove_brands.* (modules/logos/tools/brand_logos.py).
UNIT_TEST(GroveBrands_Find)
{
  auto const & pack = BrandPack::Instance();

  std::vector<std::string> const supermarket = {"shop-supermarket", "building"};
  std::vector<std::string> const fastFood = {"amenity-fast_food"};

  // A Dutch chain, found by any case of its name, only in its country and for its kind of place.
  TEST_EQUAL(pack.Find("Albert Heijn", "Netherlands", supermarket), "Q1653985", ());
  TEST_EQUAL(pack.Find("albert heijn", "Netherlands", supermarket), "Q1653985", ());
  TEST_EQUAL(pack.Find("Albert Heijn", "Japan", supermarket), "", ());
  TEST_EQUAL(pack.Find("Albert Heijn", "Netherlands", fastFood), "", ());

  // A worldwide chain is found anywhere.
  TEST_EQUAL(pack.Find("McDonald's", "Netherlands", fastFood), "Q38076", ());
  TEST_EQUAL(pack.Find("McDonald's", "Brazil", fastFood), "Q38076", ());

  TEST_EQUAL(pack.Find("No Such Brand Anywhere", "Netherlands", supermarket), "", ());

  // Logo colours: McDonald's red, Albert Heijn blue.
  auto const red = pack.GetColor("Q38076");
  TEST_GREATER((red >> 16) & 0xFF, 0xB0, (red));
  TEST_LESS(red & 0xFF, 0x40, (red));
  auto const blue = pack.GetColor("Q1653985");
  TEST_GREATER(blue & 0xFF, 0xB0, (blue));
  TEST_EQUAL(pack.GetColor("Q0"), 0, ());

  TEST(!pack.ReadBadge("Q1653985").empty(), ());
  TEST(pack.ReadBadge("Q0").empty(), ());
}

UNIT_TEST(GroveBrands_SlotsRunOut)
{
  // Room for two badges.
  grove::BrandIndex index({64, 32}, 32);
  TEST(!index.Prepare("Q0"), ("Unknown brand"));
  TEST(index.Prepare("Q1653985"), ());
  TEST(index.Prepare("Q1653985"), ("Already there"));
  TEST(index.Prepare("Q38076"), ());
  TEST(!index.Prepare("Q37158"), ("No slot left"));

  bool isNew = true;
  auto const info = index.MapResource(grove::BrandKey("Q38076"), isNew);
  TEST(info != nullptr, ());
  auto const & r = info->GetTexRect();
  TEST_ALMOST_EQUAL_ABS(r.minX(), 0.5f, 1e-6f, ());
  TEST_ALMOST_EQUAL_ABS(r.maxX(), 1.0f, 1e-6f, ());
  TEST_ALMOST_EQUAL_ABS(r.minY(), 0.0f, 1e-6f, ());
  TEST_ALMOST_EQUAL_ABS(r.maxY(), 1.0f, 1e-6f, ());
}
}  // namespace grove_brand_tests
