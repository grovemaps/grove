#pragma once

#include "indexer/map_object.hpp"

#include "geometry/latlon.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace grove::reviews
{
// Reviews of places from Mangrove (https://mangrove.reviews), an open review commons (CC BY 4.0), as CoMaps shows
// them. CoMaps builds them into its map files; Grove uses Organic Maps' map files, so it asks Mangrove's API for the
// reviews of the place whose card opens, after showing those in the bundled pack (data/grove_reviews.bin, built by
// modules/reviews/tools/mangrove_reviews.py), which works offline. Mangrove identifies a place by its "subject", a geo
// URI with the place's name and how far its area reaches: "geo:52.3562,4.9115?q=Veganees&u=50".

struct Subject
{
  ms::LatLon m_point;
  std::string m_name;
  uint32_t m_uncertainty = 0;  // Meters.
};

struct Review
{
  uint8_t m_rating = 0;  // 0..100.
  std::string m_opinion;
  std::string m_author;
  int64_t m_time = 0;  // Seconds since 1970.
};

// How the last half year's reviews rate the place, when that differs from the older ones by half a star or more.
struct Trend
{
  float m_recentStars = 0;
  size_t m_recentCount = 0;  // Rated reviews in the last half year.
  bool m_better = false;
};

struct PlaceReviews
{
  std::vector<Review> m_reviews;  // Newest first.

  // 1..5 stars, 0 without ratings.
  float AverageStars() const;

  // Needs at least two rated reviews in the last half year and two older ones. now: seconds since 1970.
  std::optional<Trend> RecentTrend(int64_t now) const;

  // Adds the other's reviews that aren't here yet (same time, rating and author), keeping newest first.
  void Merge(PlaceReviews const & other);
};

// The place's subject, or nothing for places that people don't review (benches, roads, cities...). Ported from
// CoMaps' libs/editor/review.cpp.
std::optional<Subject> GetSubject(osm::MapObject const & place);

std::string ToUri(Subject const & subject);
std::optional<Subject> FromUri(std::string_view uri);

// Mangrove's web page for writing a review of the place.
std::string WriteReviewUrl(Subject const & subject);

// The reviews of the place in a response of Mangrove's /geo request.
PlaceReviews ParseReviews(std::string const & json, Subject const & place);

// The reviews of the place in a cell of the bundled pack: lines of "lat lon uncertainty name rating time author
// opinion", tab-separated, with \t, \n and \\ escaped.
PlaceReviews ParsePackCell(std::string_view cell, Subject const & place);

// The place's reviews in the bundled pack.
PlaceReviews FindBundled(Subject const & place);

// Asks Mangrove for the place's reviews; blocks until it answers. Nothing on network errors.
std::optional<PlaceReviews> Fetch(Subject const & place);

// The switch, Feature::Reviews (on by default).
bool IsEnabled();
void SetEnabled(bool enabled);
}  // namespace grove::reviews
