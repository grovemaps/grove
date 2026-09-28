#include "map/grove_tripadvisor.hpp"

#include "indexer/search_string_utils.hpp"

#include "platform/http_client.hpp"
#include "platform/settings.hpp"

#include "coding/url.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <sstream>

#include <glaze/json.hpp>

namespace grove::tripadvisor
{
// Tripadvisor's JSON, as far as Grove reads it (glaze needs these types to have linkage).
namespace json
{
struct Location
{
  std::string location_id;
  std::string name;
};

struct Nearby
{
  std::vector<Location> data;
};

struct Details
{
  std::string name;
  std::optional<std::string> web_url;
  std::optional<std::string> rating;
  std::optional<std::string> num_reviews;
  std::optional<std::string> latitude;
  std::optional<std::string> longitude;
};

struct User
{
  std::optional<std::string> username;
};

struct Review
{
  std::optional<int> rating;
  std::optional<std::string> title;
  std::optional<std::string> text;
  std::optional<std::string> published_date;
  std::optional<User> user;
};

struct Reviews
{
  std::vector<Review> data;
};
}  // namespace json

namespace
{
std::string_view constexpr kKeySetting = "GroveTripadvisorKey";
std::string_view constexpr kApi = "https://api.content.tripadvisor.com/api/v1/location/";
// How far Tripadvisor's point may be from the map's, beyond the place's own reach.
double constexpr kMatchMeters = 150;
int64_t constexpr kRecentSeconds = 183 * 24 * 3600;

glz::opts constexpr kOpts{.error_on_unknown_keys = false, .error_on_missing_keys = false};

// Letters and digits, lower case and without accents, as search compares names.
strings::UniString Letters(std::string_view s)
{
  strings::UniString result;
  for (auto const c : search::NormalizeAndSimplifyString(s))
    if (c >= 0x80 || std::isalnum(static_cast<int>(c)))
      result.push_back(c);
  return result;
}

std::optional<std::string> Get(std::string const & url)
{
  platform::HttpClient request(url);
  request.SetRawHeader("Accept", "application/json");
  request.SetTimeout(15);
  std::string response;
  if (!request.RunHttpRequest(response))
  {
    LOG(LWARNING, ("Tripadvisor request failed:", request.ErrorCode()));
    return {};
  }
  return response;
}
}  // namespace

std::optional<reviews::Trend> Ratings::RecentTrend(int64_t now) const
{
  size_t count = 0;
  double sum = 0;
  for (auto const & r : m_reviews.m_reviews)
  {
    if (r.m_rating > 0 && r.m_time >= now - kRecentSeconds)
    {
      sum += 1 + r.m_rating / 25.0;
      ++count;
    }
  }
  if (count < 2 || m_stars == 0)
    return {};
  float const recent = static_cast<float>(sum / count);
  if (std::abs(recent - m_stars) < 0.5f)
    return {};
  return reviews::Trend{recent, count, recent > m_stars};
}

std::string GetKey()
{
  std::string key;
  settings::TryGet(kKeySetting, key);
  return key;
}

void SetKey(std::string const & key)
{
  std::string trimmed = key;
  strings::Trim(trimmed);
  settings::Set(kKeySetting, trimmed);
}

bool SameName(std::string_view a, std::string_view b)
{
  auto const x = Letters(a), y = Letters(b);
  if (x.empty() || y.empty())
    return false;
  if (x == y)
    return true;
  auto const & shorter = x.size() < y.size() ? x : y;
  auto const & longer = x.size() < y.size() ? y : x;
  return shorter.size() >= 4 &&
         std::search(longer.begin(), longer.end(), shorter.begin(), shorter.end()) != longer.end();
}

std::optional<std::string> ParseNearby(std::string const & json, reviews::Subject const & place)
{
  json::Nearby nearby;
  if (auto const error = glz::read<kOpts>(nearby, json); error)
    return {};
  // Nearest first, as the API sorts them.
  for (auto const & l : nearby.data)
    if (SameName(l.name, place.m_name))
      return l.location_id;
  return {};
}

std::optional<Ratings> ParseDetails(std::string const & json, reviews::Subject const & place)
{
  json::Details details;
  if (auto const error = glz::read<kOpts>(details, json); error)
    return {};
  double lat, lon;
  if (details.latitude && details.longitude && strings::to_double(*details.latitude, lat) &&
      strings::to_double(*details.longitude, lon) &&
      ms::DistanceOnEarth({lat, lon}, place.m_point) > place.m_uncertainty + kMatchMeters)
    return {};

  Ratings ratings;
  ratings.m_name = details.name;
  ratings.m_url = details.web_url.value_or("");
  double stars;
  if (details.rating && strings::to_double(*details.rating, stars))
    ratings.m_stars = static_cast<float>(stars);
  uint32_t count;
  if (details.num_reviews && strings::to_uint(*details.num_reviews, count))
    ratings.m_count = count;
  return ratings;
}

reviews::PlaceReviews ParseReviews(std::string const & json)
{
  json::Reviews parsed;
  reviews::PlaceReviews result;
  if (auto const error = glz::read<kOpts>(parsed, json); error)
    return result;
  for (auto const & r : parsed.data)
  {
    reviews::Review review;
    // Tripadvisor's 1..5 bubbles on Mangrove's scale, where 5 stars is 100 and 0 means no rating: 1 bubble is 1.
    if (r.rating && *r.rating >= 1)
      review.m_rating = static_cast<uint8_t>(std::max(1, (std::min(*r.rating, 5) - 1) * 25));
    std::string opinion = r.title.value_or("");
    if (r.text && !r.text->empty())
      opinion += (opinion.empty() ? "" : "\n") + *r.text;
    review.m_opinion = std::move(opinion);
    review.m_author = r.user && r.user->username ? *r.user->username : "";
    if (r.published_date)
      if (auto const t = base::StringToTimestamp(*r.published_date); t != base::INVALID_TIME_STAMP)
        review.m_time = t;
    result.m_reviews.push_back(std::move(review));
  }
  std::ranges::sort(result.m_reviews, std::ranges::greater{}, &reviews::Review::m_time);
  return result;
}

std::optional<Ratings> Fetch(reviews::Subject const & place, std::string const & language)
{
  std::string const key = GetKey();
  if (key.empty() || place.m_name.empty())
    return {};
  std::string const common = "key=" + url::UrlEncode(key) + "&language=" + url::UrlEncode(language);

  std::ostringstream nearbyUrl;
  nearbyUrl << std::setprecision(8) << kApi << "nearby_search?latLong=" << place.m_point.m_lat << "%2C"
            << place.m_point.m_lon << "&radius=0.3&radiusUnit=km&" << common;
  auto const nearby = Get(nearbyUrl.str());
  if (!nearby)
    return {};
  auto const id = ParseNearby(*nearby, place);
  if (!id)
    return {};

  auto const details = Get(std::string(kApi) + url::UrlEncode(*id) + "/details?" + common);
  if (!details)
    return {};
  auto ratings = ParseDetails(*details, place);
  if (!ratings)
    return {};
  if (auto const reviews = Get(std::string(kApi) + url::UrlEncode(*id) + "/reviews?" + common))
    ratings->m_reviews = ParseReviews(*reviews);
  return ratings;
}
}  // namespace grove::tripadvisor
