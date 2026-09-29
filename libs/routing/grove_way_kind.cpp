#include "routing/grove_way_kind.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_data.hpp"

#include "base/assert.hpp"

#include <utility>
#include <vector>

namespace grove
{
namespace
{
// Types and their kinds, the more specific first: a crossing is a footway too.
std::vector<std::pair<uint32_t, WayKind>> const & Kinds()
{
  static auto const kinds = []
  {
    auto const & c = classif();
    return std::vector<std::pair<uint32_t, WayKind>>{
        {c.GetTypeByPath({"highway", "footway", "crossing"}), WayKind::None},
        {c.GetTypeByPath({"highway", "footway", "sidewalk"}), WayKind::None},
        {c.GetTypeByPath({"highway", "footway", "bicycle"}), WayKind::SharedPath},
        {c.GetTypeByPath({"highway", "path", "bicycle"}), WayKind::SharedPath},
        {c.GetTypeByPath({"highway", "cycleway"}), WayKind::BikePath},
        {c.GetTypeByPath({"highway", "footway"}), WayKind::Path},
        {c.GetTypeByPath({"highway", "path"}), WayKind::Path},
        {c.GetTypeByPath({"highway", "bridleway"}), WayKind::Path},
        {c.GetTypeByPath({"highway", "steps"}), WayKind::Steps},
        {c.GetTypeByPath({"highway", "track"}), WayKind::Track},
    };
  }();
  return kinds;
}
}  // namespace

WayKind GetWayKind(feature::TypesHolder const & types)
{
  for (auto const & [kindType, kind] : Kinds())
  {
    uint8_t const level = ftype::GetLevel(kindType);
    for (uint32_t const t : types)
      if (ftype::Trunc(t, level) == kindType)
        return kind;
  }
  return WayKind::None;
}

bool IsSameWayKind(WayKind a, WayKind b)
{
  auto const alike = [](WayKind shared, WayKind other)
  { return shared == WayKind::SharedPath && (other == WayKind::BikePath || other == WayKind::Path); };
  return a == b || alike(a, b) || alike(b, a);
}

std::string WayKindTextId(WayKind kind, bool pedestrian)
{
  switch (kind)
  {
  case WayKind::None: return {};
  case WayKind::BikePath: return "grove_the_bike_path";
  case WayKind::SharedPath: return pedestrian ? "grove_the_path" : "grove_the_bike_path";
  case WayKind::Path: return "grove_the_path";
  case WayKind::Steps: return "grove_the_stairs";
  case WayKind::Track: return "grove_the_track";
  }
  UNREACHABLE();
}

std::string DebugPrint(WayKind kind)
{
  switch (kind)
  {
  case WayKind::None: return "None";
  case WayKind::BikePath: return "BikePath";
  case WayKind::SharedPath: return "SharedPath";
  case WayKind::Path: return "Path";
  case WayKind::Steps: return "Steps";
  case WayKind::Track: return "Track";
  }
  UNREACHABLE();
}
}  // namespace grove
