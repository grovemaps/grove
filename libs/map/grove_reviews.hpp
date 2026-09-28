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
// reviews of the place whose card opens. Mangrove identifies a place by its "subject", a geo URI with the place's
// name and how far its area reaches: "geo:52.3562,4.9115?q=Veganees&u=50".

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

struct PlaceReviews
{
  std::vector<Review> m_reviews;  // Newest first.

  // 1..5 stars.
  float AverageStars() const;
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

// Asks Mangrove for the place's reviews; blocks until it answers. Nothing on network errors.
std::optional<PlaceReviews> Fetch(Subject const & place);

// The settings switch (key "GroveReviews", on by default).
bool IsEnabled();
void SetEnabled(bool enabled);
}  // namespace grove::reviews
