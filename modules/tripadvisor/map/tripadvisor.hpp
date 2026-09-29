#pragma once

#include "modules/reviews/map/reviews.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace grove::tripadvisor
{
// A place's rating and newest reviews from Tripadvisor's Content API (https://tripadvisor-content-api.readme.io),
// for users who enter their own API key in Settings. Nothing is stored: Tripadvisor's terms allow showing its content
// with attribution and a link to the place's Tripadvisor page, not keeping it. Opening a place card sends the place's
// position and name to Tripadvisor, so it only happens with a key set.

struct Ratings
{
  std::string m_name;
  std::string m_url;  // The place's Tripadvisor page, which the card must link to.
  float m_stars = 0;  // 1..5, 0 without a rating.
  uint32_t m_count = 0;
  reviews::PlaceReviews m_reviews;  // The newest, at most 5; ratings on Mangrove's 0..100 scale.

  // How the newest reviews differ from the overall rating: at least two from the last half year, half a star or more.
  std::optional<reviews::Trend> RecentTrend(int64_t now) const;
};

// The API key (settings key "GroveTripadvisorKey"), empty for none.
std::string GetKey();
void SetKey(std::string const & key);

// Parts of the API's answers, for tests: the best matching location id near the place, the details, the reviews.
std::optional<std::string> ParseNearby(std::string const & json, reviews::Subject const & place);
std::optional<Ratings> ParseDetails(std::string const & json, reviews::Subject const & place);
reviews::PlaceReviews ParseReviews(std::string const & json);

// Whether two names are the same place's: the same letters and digits in any case, or one within the other.
bool SameName(std::string_view a, std::string_view b);

// Asks Tripadvisor for the place; blocks. Nothing without a key, a match or on network errors.
std::optional<Ratings> Fetch(reviews::Subject const & place, std::string const & language);
}  // namespace grove::tripadvisor
