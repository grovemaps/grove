#include "testing/testing.hpp"

#include "modules/logos/drape_frontend/brand_layer.hpp"

#include <vector>

namespace grove_brand_layer_tests
{
using grove::BrandPlace;
using grove::ClusterBrandPlaces;

// Wikidata ids from data/grove_brands.txt: Albert Heijn, McDonald's.
auto constexpr kAH = "Q1653985";
auto constexpr kMcD = "Q38076";

BrandPlace Place(uint32_t index, double x, double y, char const * brand)
{
  return {FeatureID(MwmSet::MwmId(), index), {x, y}, brand};
}

UNIT_TEST(GroveBrandLayer_Bundles)
{
  // Two chains next to each other, a second Albert Heijn close by, and one far away.
  std::vector<BrandPlace> const places = {Place(1, 0.0, 0.0, kMcD), Place(2, 0.4, 0.1, kAH), Place(3, 0.6, 0.3, kAH),
                                          Place(4, 10.0, 10.0, kAH)};
  auto const clusters = ClusterBrandPlaces(places, 1.0 /* distance */, 3 /* maxLogos */);
  TEST_EQUAL(clusters.size(), 2, ());

  auto const & near = clusters[0].m_logos.size() == 2 ? clusters[0] : clusters[1];
  auto const & far = clusters[0].m_logos.size() == 2 ? clusters[1] : clusters[0];

  // One logo per chain: the second Albert Heijn only moves the row's center.
  TEST_EQUAL(near.m_logos.size(), 2, ());
  // More common chains first: McDonald's has more places worldwide.
  TEST_EQUAL(near.m_logos[0]->m_brand, kMcD, ());
  TEST_EQUAL(near.m_logos[1]->m_brand, kAH, ());
  TEST_ALMOST_EQUAL_ABS(near.m_center.x, 1.0 / 3, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(near.m_center.y, 0.4 / 3, 1e-9, ());

  TEST_EQUAL(far.m_logos.size(), 1, ());
  TEST_EQUAL(far.m_logos[0]->m_id.m_index, 4, ());
}

UNIT_TEST(GroveBrandLayer_StableAndCapped)
{
  std::vector<BrandPlace> const places = {Place(1, 0.0, 0.0, kMcD), Place(2, 0.1, 0.0, kAH), Place(3, 0.2, 0.0, "Q0"),
                                          Place(4, 0.3, 0.0, "Q1")};
  auto const clusters = ClusterBrandPlaces(places, 1.0, 2 /* maxLogos */);
  TEST_EQUAL(clusters.size(), 1, ());
  TEST_EQUAL(clusters[0].m_logos.size(), 2, ());

  // The same places in another order make the same clusters.
  std::vector<BrandPlace> const reversedPlaces(places.rbegin(), places.rend());
  auto const reversed = ClusterBrandPlaces(reversedPlaces, 1.0, 2);
  TEST_EQUAL(reversed.size(), 1, ());
  for (size_t i = 0; i < 2; ++i)
    TEST_EQUAL(reversed[0].m_logos[i]->m_id, clusters[0].m_logos[i]->m_id, (i));
}
UNIT_TEST(GroveBrandLayer_ChainSpacing)
{
  // Two McDonald's 3 apart, and one far away: zoomed out, the second one is left out.
  std::vector<BrandPlace> const places = {Place(1, 0.0, 0.0, kMcD), Place(2, 3.0, 0.0, kMcD), Place(3, 20.0, 0.0, kMcD),
                                          Place(4, 3.2, 0.0, kAH)};
  TEST_EQUAL(ClusterBrandPlaces(places, 1.0, 3).size(), 3, ());
  auto const spaced = ClusterBrandPlaces(places, 1.0, 3, 5.0 /* chainSpacing */);
  size_t logos = 0;
  for (auto const & c : spaced)
  {
    logos += c.m_logos.size();
    for (auto const * p : c.m_logos)
      TEST_NOT_EQUAL(p->m_id.m_index, 2, ());
  }
  TEST_EQUAL(logos, 3, ("Other chains stay"));
}
}  // namespace grove_brand_layer_tests
