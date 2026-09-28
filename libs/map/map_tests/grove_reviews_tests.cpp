#include "testing/testing.hpp"

#include "map/grove_reviews.hpp"

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
}  // namespace grove_reviews_tests
