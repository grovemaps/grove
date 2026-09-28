#include "map/grove_reviews.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_data.hpp"

#include "platform/http_client.hpp"
#include "platform/settings.hpp"

#include "coding/url.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

#include <glaze/json.hpp>

namespace grove::reviews
{
// Mangrove's JSON, as far as Grove reads it (glaze needs these types to have linkage).
namespace json
{
struct JsonMetadata
{
  std::optional<std::string> nickname;
};

struct JsonPayload
{
  std::string sub;
  std::optional<int> rating;
  std::optional<std::string> opinion;
  int64_t iat = 0;
  std::optional<JsonMetadata> metadata;
};

struct JsonReview
{
  JsonPayload payload;
};

struct JsonResponse
{
  std::vector<JsonReview> reviews;
};
}  // namespace json

namespace
{
std::string_view constexpr kEnabledKey = "GroveReviews";
std::string_view constexpr kApi = "https://api.mangrove.reviews";

// Uncertainty of a point place, as MapComplete and CoMaps use.
uint32_t constexpr kPointUncertaintyMeters = 10;
// How far a review's point may be from the place, beyond both uncertainties: reviewers pin places by hand.
double constexpr kMatchSlackMeters = 30;

// Place types people review, from CoMaps' libs/editor/review.cpp (commit 9476f2f0), Apache License 2.0.
std::string_view constexpr kReviewedTypes[] = {"amenity-bank",
                                               "amenity-bar",
                                               "amenity-bicycle_rental",
                                               "amenity-biergarten",
                                               "amenity-bureau_de_change",
                                               "amenity-cafe",
                                               "amenity-car_rental",
                                               "amenity-car_wash",
                                               "amenity-casino",
                                               "amenity-childcare",
                                               "amenity-cinema",
                                               "amenity-clinic",
                                               "amenity-college",
                                               "amenity-community_centre",
                                               "amenity-community_centre-youth_centre",
                                               "amenity-conference_centre",
                                               "amenity-dentist",
                                               "amenity-doctors",
                                               "amenity-driving_school",
                                               "amenity-events_venue",
                                               "amenity-exhibition_centre",
                                               "amenity-fast_food",
                                               "amenity-flight_school",
                                               "amenity-food_court",
                                               "amenity-fuel",
                                               "amenity-hospital",
                                               "amenity-ice_cream",
                                               "amenity-internet_cafe",
                                               "amenity-kindergarten",
                                               "amenity-language_school",
                                               "amenity-library",
                                               "amenity-love_hotel",
                                               "amenity-marketplace",
                                               "amenity-money_transfer",
                                               "amenity-motorcycle_parking",
                                               "amenity-motorcycle_rental",
                                               "amenity-music_school",
                                               "amenity-nightclub",
                                               "amenity-nursing_home",
                                               "amenity-parking",
                                               "amenity-pharmacy",
                                               "amenity-planetarium",
                                               "amenity-police",
                                               "amenity-post_office",
                                               "amenity-prep_school",
                                               "amenity-pub",
                                               "amenity-public_bath",
                                               "amenity-recycling-centre",
                                               "amenity-restaurant",
                                               "amenity-sailing_school",
                                               "amenity-school",
                                               "amenity-social_facility",
                                               "amenity-theatre",
                                               "amenity-toilets",
                                               "amenity-townhall",
                                               "amenity-university",
                                               "amenity-vehicle_inspection",
                                               "amenity-veterinary",
                                               "craft-beekeeper",
                                               "craft-blacksmith",
                                               "craft-brewery",
                                               "craft-carpenter",
                                               "craft-caterer",
                                               "craft-confectionery",
                                               "craft-electrician",
                                               "craft-electronics_repair",
                                               "craft-gardener",
                                               "craft-grinding_mill",
                                               "craft-handicraft",
                                               "craft-hvac",
                                               "craft-key_cutter",
                                               "craft-locksmith",
                                               "craft-metal_construction",
                                               "craft-painter",
                                               "craft-photographer",
                                               "craft-plumber",
                                               "craft-sawmill",
                                               "craft-shoemaker",
                                               "craft-tailor",
                                               "craft-winery",
                                               "healthcare-physiotherapist",
                                               "healthcare-psychotherapist",
                                               "leisure-amusement_arcade",
                                               "leisure-bowling_alley",
                                               "leisure-dance",
                                               "leisure-dog_park",
                                               "leisure-escape_game",
                                               "leisure-fitness_centre",
                                               "leisure-fitness_centre-sport-yoga",
                                               "leisure-fitness_station",
                                               "leisure-garden",
                                               "leisure-golf_course",
                                               "leisure-hackerspace",
                                               "leisure-indoor_play",
                                               "leisure-marina",
                                               "leisure-miniature_golf",
                                               "leisure-nature_reserve",
                                               "leisure-park",
                                               "leisure-pitch",
                                               "leisure-playground",
                                               "leisure-resort",
                                               "leisure-sauna",
                                               "leisure-sports_centre",
                                               "leisure-sports_centre-sport-swimming",
                                               "leisure-sports_hall",
                                               "leisure-stadium",
                                               "leisure-swimming_pool",
                                               "leisure-track",
                                               "leisure-water_park",
                                               "natural-beach",
                                               "office-company",
                                               "office-diplomatic",
                                               "office-estate_agent",
                                               "office-government",
                                               "office-insurance",
                                               "office-lawyer",
                                               "office-ngo",
                                               "office-security",
                                               "office-telecommunication",
                                               "shop-agrarian",
                                               "shop-alcohol",
                                               "shop-antiques",
                                               "shop-appliance",
                                               "shop-art",
                                               "shop-auction",
                                               "shop-baby_goods",
                                               "shop-bag",
                                               "shop-bakery",
                                               "shop-bathroom_furnishing",
                                               "shop-beauty",
                                               "shop-beauty-day_spa",
                                               "shop-beauty-nails",
                                               "shop-bed",
                                               "shop-beverages",
                                               "shop-bicycle",
                                               "shop-bookmaker",
                                               "shop-books",
                                               "shop-boutique",
                                               "shop-butcher",
                                               "shop-camera",
                                               "shop-cannabis",
                                               "shop-car_parts",
                                               "shop-car_repair",
                                               "shop-car_repair-tyres",
                                               "shop-caravan",
                                               "shop-carpet",
                                               "shop-car",
                                               "shop-charity",
                                               "shop-cheese",
                                               "shop-chemist",
                                               "shop-chocolate",
                                               "shop-clothes",
                                               "shop-coffee",
                                               "shop-collector",
                                               "shop-computer",
                                               "shop-confectionery",
                                               "shop-convenience",
                                               "shop-copyshop",
                                               "shop-cosmetics",
                                               "shop-craft",
                                               "shop-curtain",
                                               "shop-dairy",
                                               "shop-deli",
                                               "shop-department_store",
                                               "shop-doityourself",
                                               "shop-dry_cleaning",
                                               "shop-electrical",
                                               "shop-electronics",
                                               "shop-erotic",
                                               "shop-fabric",
                                               "shop-farm",
                                               "shop-fashion_accessories",
                                               "shop-fishing",
                                               "shop-florist",
                                               "shop-funeral_directors",
                                               "shop-furniture",
                                               "shop-garden_centre",
                                               "shop-gas",
                                               "shop-gift",
                                               "shop-greengrocer",
                                               "shop-grocery",
                                               "shop-hairdresser",
                                               "shop-hardware",
                                               "shop-health_food",
                                               "shop-hearing_aids",
                                               "shop-herbalist",
                                               "shop-hifi",
                                               "shop-houseware",
                                               "shop-interior_decoration",
                                               "shop-jewelry",
                                               "shop-kiosk",
                                               "shop-kitchen",
                                               "shop-laundry",
                                               "shop-lighting",
                                               "shop-lottery",
                                               "shop-mall",
                                               "shop-massage",
                                               "shop-medical_supply",
                                               "shop-mobile_phone",
                                               "shop-money_lender",
                                               "shop-motorcycle_repair",
                                               "shop-motorcycle",
                                               "shop-musical_instrument",
                                               "shop-music",
                                               "shop-newsagent",
                                               "shop-nutrition_supplements",
                                               "shop-optician",
                                               "shop-outdoor",
                                               "shop-outpost",
                                               "shop-paint",
                                               "shop-pasta",
                                               "shop-pawnbroker",
                                               "shop-perfumery",
                                               "shop-pet_grooming",
                                               "shop-pet",
                                               "shop-photo",
                                               "shop-seafood",
                                               "shop-second_hand",
                                               "shop-sewing",
                                               "shop-shoes",
                                               "shop-sports",
                                               "shop-stationery",
                                               "shop-storage_rental",
                                               "shop-supermarket",
                                               "shop-tattoo",
                                               "shop-tea",
                                               "shop-telecommunication",
                                               "shop-ticket",
                                               "shop-tobacco",
                                               "shop-toys",
                                               "shop-trade",
                                               "shop-travel_agency",
                                               "shop-tyres",
                                               "shop-variety_store",
                                               "shop-video_games",
                                               "shop-video",
                                               "shop-watches",
                                               "shop-water",
                                               "shop-wholesale",
                                               "shop-wine",
                                               "tourism-alpine_hut",
                                               "tourism-apartment",
                                               "tourism-aquarium",
                                               "tourism-artwork",
                                               "tourism-artwork-painting",
                                               "tourism-artwork-sculpture",
                                               "tourism-artwork-statue",
                                               "tourism-attraction",
                                               "tourism-camp_site",
                                               "tourism-caravan_site",
                                               "tourism-chalet",
                                               "tourism-gallery",
                                               "tourism-guest_house",
                                               "tourism-hostel",
                                               "tourism-hotel",
                                               "tourism-information",
                                               "tourism-information-office",
                                               "tourism-information-visitor_centre",
                                               "tourism-motel",
                                               "tourism-museum",
                                               "tourism-picnic_site",
                                               "tourism-theme_park",
                                               "tourism-viewpoint",
                                               "tourism-wilderness_hut",
                                               "tourism-zoo"};

bool IsReviewed(feature::TypesHolder const & types)
{
  auto const & c = classif();
  return std::ranges::any_of(types, [&c](uint32_t type)
  { return std::ranges::find(kReviewedTypes, c.GetReadableObjectName(type)) != std::end(kReviewedTypes); });
}

uint32_t Uncertainty(osm::MapObject const & place)
{
  std::vector<m2::PointD> const * points = nullptr;
  switch (place.GetGeomType())
  {
  case feature::GeomType::Line: points = &place.GetPoints(); break;
  case feature::GeomType::Area: points = &place.GetTriangesAsPoints(); break;
  default: return kPointUncertaintyMeters;
  }
  double maxDistance = 0;
  auto const center = place.GetMercator();
  for (auto const & p : *points)
    maxDistance = std::max(maxDistance, mercator::DistanceOnEarth(center, p));
  return std::max(kPointUncertaintyMeters, static_cast<uint32_t>(maxDistance));
}

bool SameName(std::string_view a, std::string_view b)
{
  strings::Trim(a);
  strings::Trim(b);
  return strings::MakeLowerCase(std::string(a)) == strings::MakeLowerCase(std::string(b));
}
}  // namespace

float PlaceReviews::AverageStars() const
{
  double sum = 0;
  size_t count = 0;
  for (auto const & r : m_reviews)
  {
    if (r.m_rating > 0)
    {
      sum += r.m_rating;
      ++count;
    }
  }
  // Mangrove's 0..100 ratings are 1..5 stars in steps of 25.
  return count == 0 ? 0 : static_cast<float>(1 + sum / count / 25);
}

std::optional<Subject> GetSubject(osm::MapObject const & place)
{
  if (!IsReviewed(place.GetTypes()))
    return {};
  return Subject{place.GetLatLon(), std::string(place.GetDefaultName()), Uncertainty(place)};
}

std::string ToUri(Subject const & subject)
{
  std::ostringstream geo;
  geo << std::setprecision(10) << "geo:" << subject.m_point.m_lat << "," << subject.m_point.m_lon << "?";
  if (!subject.m_name.empty())
    geo << "q=" << url::UrlEncode(subject.m_name) << "&";
  geo << "u=" << subject.m_uncertainty;
  return geo.str();
}

std::optional<Subject> FromUri(std::string_view uri)
{
  if (!uri.starts_with("geo:"))
    return {};
  uri.remove_prefix(4);
  auto const query = uri.find('?');
  std::string_view const point = uri.substr(0, query);
  auto const comma = point.find(',');
  Subject subject;
  if (comma == std::string_view::npos || !strings::to_double(point.substr(0, comma), subject.m_point.m_lat) ||
      !strings::to_double(point.substr(comma + 1), subject.m_point.m_lon))
    return {};

  if (query != std::string_view::npos)
  {
    strings::Tokenize(uri.substr(query + 1), "&", [&subject](std::string_view param)
    {
      if (param.starts_with("q="))
        subject.m_name = url::UrlDecode(param.substr(2));
      else if (param.starts_with("u="))
        UNUSED_VALUE(strings::to_uint(param.substr(2), subject.m_uncertainty));
    });
  }
  return subject;
}

std::string WriteReviewUrl(Subject const & subject)
{
  return "https://mangrove.reviews/search?sub=" + url::UrlEncode(ToUri(subject));
}

PlaceReviews ParseReviews(std::string const & json, Subject const & place)
{
  json::JsonResponse response;
  glz::opts constexpr opts{.error_on_unknown_keys = false, .error_on_missing_keys = false};
  if (auto const error = glz::read<opts>(response, json); error)
  {
    LOG(LWARNING, ("Can't parse Mangrove reviews:", glz::format_error(error, json)));
    return {};
  }

  PlaceReviews result;
  for (auto const & r : response.reviews)
  {
    auto const & p = r.payload;
    auto const subject = FromUri(p.sub);
    if (!subject)
      continue;
    double const distance = ms::DistanceOnEarth(subject->m_point, place.m_point);
    if (distance > std::max(subject->m_uncertainty, place.m_uncertainty) + kMatchSlackMeters)
      continue;
    // A review of another place nearby, unless one of them has no name.
    if (!subject->m_name.empty() && !place.m_name.empty() && !SameName(subject->m_name, place.m_name))
      continue;

    Review review;
    review.m_rating = static_cast<uint8_t>(std::clamp(p.rating.value_or(0), 0, 100));
    review.m_opinion = p.opinion.value_or("");
    review.m_author = p.metadata && p.metadata->nickname ? *p.metadata->nickname : "";
    review.m_time = p.iat;
    result.m_reviews.push_back(std::move(review));
  }
  std::ranges::sort(result.m_reviews, std::ranges::greater{}, &Review::m_time);
  return result;
}

std::optional<PlaceReviews> Fetch(Subject const & place)
{
  // A box around the place, as large as its area plus the slack reviews may be off by.
  double const radius = place.m_uncertainty + kMatchSlackMeters;
  auto const rect = mercator::RectByCenterXYAndSizeInMeters(mercator::FromLatLon(place.m_point), radius);
  auto const min = mercator::ToLatLon(rect.LeftBottom());
  auto const max = mercator::ToLatLon(rect.RightTop());

  std::ostringstream url;
  url << std::setprecision(10) << kApi << "/geo?xmin=" << min.m_lon << "&ymin=" << min.m_lat << "&xmax=" << max.m_lon
      << "&ymax=" << max.m_lat;
  platform::HttpClient request(url.str());
  request.SetTimeout(15);
  std::string response;
  if (!request.RunHttpRequest(response))
  {
    LOG(LWARNING, ("Mangrove reviews request failed:", request.ErrorCode()));
    return {};
  }
  return ParseReviews(response, place);
}

bool IsEnabled()
{
  bool enabled = true;
  settings::TryGet(kEnabledKey, enabled);
  return enabled;
}

void SetEnabled(bool enabled)
{
  settings::Set(kEnabledKey, enabled);
}
}  // namespace grove::reviews
