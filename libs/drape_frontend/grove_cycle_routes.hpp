#pragma once

#include "indexer/route_relation.hpp"

#include "drape/color.hpp"

#include <optional>
#include <string_view>

namespace grove
{
// Cycle routes in the cycling layer: one line per road, coloured by the highest network the road belongs to, as on
// Mapy.com, instead of a stripe per route in the route's own colour. Routes of one network mostly share a colour or
// have none, and side by side stripes of the Dutch or Belgian node networks cover whole towns.

// 0 for international and national routes, 1 regional, 2 local and unknown, 3 mountain bike.
inline int CycleRouteLevel(feature::RouteRelationBase const & rel)
{
  if (rel.GetType() == feature::RouteRelationBase::Type::MTB)
    return 3;
  std::string_view const network = rel.GetNetwork();
  if (network == "icn" || network == "ncn")
    return 0;
  if (network == "rcn")
    return 1;
  return 2;
}

// The colour of a road's line for its best cycle route level, or nothing if that level isn't shown at the zoom:
// local routes and mountain bike trails appear from zoom 14, where there is room for them.
inline std::optional<dp::Color> CycleRouteColor(int level, int zoom)
{
  switch (level)
  {
  case 0: return dp::Color(0xB0, 0x2A, 0x78, 0xFF);
  case 1: return dp::Color(0xD2, 0x4A, 0x9A, zoom <= 12 ? 0xA0 : 0xFF);  // Node networks cover whole regions.
  case 2: return zoom >= 14 ? std::optional(dp::Color(0xE4, 0x86, 0xBC, 0xFF)) : std::nullopt;
  default: return zoom >= 14 ? std::optional(dp::Color(0xB0, 0x6A, 0x2C, 0xFF)) : std::nullopt;
  }
}
}  // namespace grove
