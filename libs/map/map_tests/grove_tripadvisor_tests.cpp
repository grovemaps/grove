#include "testing/testing.hpp"

#include "map/grove_tripadvisor.hpp"

namespace grove_tripadvisor_tests
{
using namespace grove;

reviews::Subject const kPlace{{52.3562, 4.9115}, "Veganees", 10};

UNIT_TEST(GroveTripadvisor_SameName)
{
  TEST(tripadvisor::SameName("Veganees", "VEGANEES"), ());
  TEST(tripadvisor::SameName("Café de Jaren", "Cafe-de Jaren"), ());
  TEST(tripadvisor::SameName("Restaurant Veganees Amsterdam", "Veganees"), ());
  TEST(!tripadvisor::SameName("Bar", "Barcelona Tapas"), ());  // Too short to be found inside another name.
  TEST(!tripadvisor::SameName("", "Veganees"), ());
}

UNIT_TEST(GroveTripadvisor_Parse)
{
  auto const id = tripadvisor::ParseNearby(
      R"({"data":[{"location_id":"111","name":"Other Place","distance":"0.01"},
                  {"location_id":"222","name":"Veganees","distance":"0.02"}]})",
      kPlace);
  TEST_EQUAL(id, std::optional<std::string>("222"), ());
  TEST(!tripadvisor::ParseNearby(R"({"data":[]})", kPlace), ());

  auto const details = tripadvisor::ParseDetails(
      R"({"location_id":"222","name":"Veganees","web_url":"https://www.tripadvisor.com/x","rating":"4.5",
          "num_reviews":"321","latitude":"52.3563","longitude":"4.9116","extra":{"a":1}})",
      kPlace);
  TEST(details, ());
  TEST_EQUAL(details->m_stars, 4.5f, ());
  TEST_EQUAL(details->m_count, 321, ());
  TEST_EQUAL(details->m_url, "https://www.tripadvisor.com/x", ());
  // Too far from the map's place: another one of the same name.
  TEST(!tripadvisor::ParseDetails(R"({"name":"Veganees","latitude":"52.40","longitude":"4.9116"})", kPlace), ());

  auto const reviews = tripadvisor::ParseReviews(
      R"({"data":[{"rating":2,"title":"Worse","text":"Slow.","published_date":"2026-09-01T10:00:00Z",
                   "user":{"username":"anna"}},
                  {"rating":5,"title":"Great","published_date":"2026-09-10T10:00:00Z"},
                  {"rating":1,"text":"Cold","published_date":"2026-08-01T10:00:00Z"}]})");
  TEST_EQUAL(reviews.m_reviews.size(), 3, ());
  TEST_EQUAL(reviews.m_reviews[0].m_rating, 100, ());  // Newest first.
  TEST_EQUAL(reviews.m_reviews[1].m_opinion, "Worse\nSlow.", ());
  TEST_EQUAL(reviews.m_reviews[1].m_author, "anna", ());
  TEST_EQUAL(reviews.m_reviews[1].m_rating, 25, ());
  TEST_EQUAL(reviews.m_reviews[2].m_rating, 1, ());  // One bubble still counts as a rating.
}

UNIT_TEST(GroveTripadvisor_Trend)
{
  int64_t constexpr kNow = 1'790'000'000;
  tripadvisor::Ratings ratings;
  ratings.m_stars = 4.5f;
  ratings.m_reviews.m_reviews = {{25, "", "", kNow - 10 * 86400}, {0 + 1, "", "", kNow - 20 * 86400}};
  auto const trend = ratings.RecentTrend(kNow);
  TEST(trend && !trend->m_better, ());
  TEST_LESS(trend->m_recentStars, 2.5f, ());

  ratings.m_reviews.m_reviews = {{100, "", "", kNow - 10 * 86400}, {75, "", "", kNow - 20 * 86400}};
  TEST(!ratings.RecentTrend(kNow), ());  // 4.5 against 4.5.
}
}  // namespace grove_tripadvisor_tests
