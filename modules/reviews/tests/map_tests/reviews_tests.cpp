#include "testing/testing.hpp"

#include "modules/reviews/map/reviews.hpp"

#include <string>

namespace grove_reviews_tests
{
using namespace grove::reviews;

UNIT_TEST(GroveReviews_Uri)
{
  Subject const place{{52.356249, 4.9115256}, "Café de Jaren", 50};
  std::string const uri = ToUri(place);
  TEST_EQUAL(uri, "geo:52.356249,4.9115256?q=Caf%C3%A9%20de%20Jaren&u=50", ());

  auto const parsed = FromUri(uri);
  TEST(parsed, ());
  TEST_ALMOST_EQUAL_ABS(parsed->m_point.m_lat, 52.356249, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(parsed->m_point.m_lon, 4.9115256, 1e-9, ());
  TEST_EQUAL(parsed->m_name, "Café de Jaren", ());
  TEST_EQUAL(parsed->m_uncertainty, 50, ());

  // A subject without a name or uncertainty still parses.
  TEST(FromUri("geo:52.1,4.2"), ());
  TEST(!FromUri("https://example.com"), ());
  TEST(!FromUri("geo:abc"), ());

  TEST_EQUAL(
      WriteReviewUrl(place),
      "https://mangrove.reviews/search?sub=geo%3A52.356249%2C4.9115256%3Fq%3DCaf%25C3%25A9%2520de%2520Jaren%26u%3D50",
      ());
}

UNIT_TEST(GroveReviews_Parse)
{
  // Shaped like Mangrove's /geo response, trimmed to what Grove reads.
  std::string const json = R"({"reviews":[
    {"signature":"a","payload":{"sub":"geo:52.35625,4.91153?q=Veganees&u=50","rating":100,
      "opinion":"Very good vegan food.","iat":1700000000,"metadata":{"nickname":"janvlug","is_personal_experience":true}}},
    {"signature":"b","payload":{"sub":"geo:52.35630,4.91150?q=veganees%20&u=10","rating":50,"iat":1710000000}},
    {"signature":"c","payload":{"sub":"geo:52.35625,4.91153?q=Other%20Place&u=10","rating":0,"iat":1720000000}},
    {"signature":"d","payload":{"sub":"geo:52.36000,4.92000?q=Veganees&u=10","rating":75,"iat":1730000000}},
    {"signature":"e","payload":{"sub":"https://example.com","rating":75,"iat":1740000000}}
  ],"issuers":{},"maresi_subjects":[]})";

  Subject const place{{52.356249, 4.9115256}, "Veganees", 20};
  auto const reviews = ParseReviews(json, place);

  // Another name at the same spot, the same name 800 m away and a non-place subject are someone else's.
  TEST_EQUAL(reviews.m_reviews.size(), 2, ());
  TEST_EQUAL(reviews.m_reviews[0].m_time, 1710000000, ("Newest first"));
  TEST_EQUAL(reviews.m_reviews[0].m_opinion, "", ());
  TEST_EQUAL(reviews.m_reviews[1].m_author, "janvlug", ());
  TEST_EQUAL(reviews.m_reviews[1].m_rating, 100, ());
  // 100 and 50 of 100: 5 and 3 stars.
  TEST_ALMOST_EQUAL_ABS(reviews.AverageStars(), 4.0f, 1e-6f, ());

  TEST(ParseReviews("not json", place).m_reviews.empty(), ());
  TEST_EQUAL(PlaceReviews{}.AverageStars(), 0.0f, ());
}

UNIT_TEST(GroveReviews_PackCell)
{
  Subject const place{{52.3562, 4.9115}, "Veganees", 10};
  // The same place, another place nearby, the place without a name in the review, and one far away.
  std::string const cell =
      "52.3562100\t4.9115200\t30\tVeganees\t100\t1700000000\tanna\tGreat\\tfood\\nand \\\\ service\n"
      "52.3562100\t4.9115200\t30\tOther Cafe\t25\t1700000100\tbob\tMeh\n"
      "52.3561900\t4.9114800\t10\t\t75\t1700000200\t\t\n"
      "52.3700000\t4.9115200\t10\tVeganees\t50\t1700000300\tcarl\tFar\n"
      "broken line\n";
  auto const reviews = ParsePackCell(cell, place);
  TEST_EQUAL(reviews.m_reviews.size(), 2, ());
  TEST_EQUAL(reviews.m_reviews[0].m_rating, 75, ());  // Newest first.
  TEST(reviews.m_reviews[0].m_author.empty(), ());
  TEST(reviews.m_reviews[0].m_opinion.empty(), ());
  TEST_EQUAL(reviews.m_reviews[1].m_opinion, "Great\tfood\nand \\ service", ());
  TEST_EQUAL(reviews.m_reviews[1].m_author, "anna", ());
}

UNIT_TEST(GroveReviews_Merge)
{
  PlaceReviews bundled{{{100, "a", "x", 10}, {50, "b", "y", 5}}};
  PlaceReviews const online{{{75, "new", "z", 20}, {100, "a", "x", 10}}};
  bundled.Merge(online);
  TEST_EQUAL(bundled.m_reviews.size(), 3, ());
  TEST_EQUAL(bundled.m_reviews[0].m_opinion, "new", ());
  TEST_EQUAL(bundled.m_reviews[2].m_opinion, "b", ());
}

UNIT_TEST(GroveReviews_Trend)
{
  int64_t constexpr kDay = 24 * 3600;
  int64_t constexpr kNow = 1'800'000'000;
  // Two recent 2-star reviews after two older 5-star ones: worse.
  PlaceReviews reviews{{{25, "", "", kNow - 10 * kDay},
                        {25, "", "", kNow - 30 * kDay},
                        {100, "", "", kNow - 400 * kDay},
                        {100, "", "", kNow - 500 * kDay}}};
  auto trend = reviews.RecentTrend(kNow);
  TEST(trend, ());
  TEST_ALMOST_EQUAL_ABS(trend->m_recentStars, 2.0f, 1e-5f, ());
  TEST_EQUAL(trend->m_recentCount, 2, ());
  TEST(!trend->m_better, ());

  // Better.
  for (auto & r : reviews.m_reviews)
    r.m_rating = 125 - r.m_rating;
  trend = reviews.RecentTrend(kNow);
  TEST(trend && trend->m_better, ());

  // Too few recent reviews.
  reviews.m_reviews.erase(reviews.m_reviews.begin());
  TEST(!reviews.RecentTrend(kNow), ());

  // Unrated reviews don't count, and small changes show nothing.
  PlaceReviews steady{{{75, "", "", kNow - kDay},
                       {0, "", "", kNow - 2 * kDay},
                       {75, "", "", kNow - 3 * kDay},
                       {100, "", "", kNow - 400 * kDay},
                       {75, "", "", kNow - 500 * kDay},
                       {75, "", "", kNow - 600 * kDay}}};
  TEST(!steady.RecentTrend(kNow), ());
}

UNIT_TEST(GroveReviews_Bundled)
{
  // Mangrove has reviews of Veganees in Amsterdam since 2024; the pack must find them.
  auto const reviews = FindBundled({{52.3562, 4.9115}, "Veganees", 10});
  TEST_GREATER(reviews.m_reviews.size(), 0, ());
}
}  // namespace grove_reviews_tests
