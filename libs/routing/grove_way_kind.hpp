#pragma once

#include <cstdint>
#include <string>

namespace feature
{
class TypesHolder;
}

namespace grove
{
// What a way is, so that voice guidance can say "turn right onto the bike path" where the way has no name, or street
// names are not announced. Streets, pedestrian ones too, crossings and pavements have none.
enum class WayKind : uint8_t
{
  None,
  BikePath,
  // A path for walking and cycling: a bike path to riders, a path to walkers.
  SharedPath,
  Path,
  Steps,
  Track,
};

WayKind GetWayKind(feature::TypesHolder const & types);

// Whether voice guidance calls both alike: a path for walking and cycling is a bike path or a path.
bool IsSameWayKind(WayKind a, WayKind b);

// The sound string of "the bike path" and so on, empty for None.
std::string WayKindTextId(WayKind kind, bool pedestrian);

std::string DebugPrint(WayKind kind);
}  // namespace grove
