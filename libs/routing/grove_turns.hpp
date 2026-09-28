#pragma once

#include "routing/car_directions.hpp"
#include "routing/route.hpp"

#include <vector>

namespace grove
{
// Cleans up bicycle turn instructions. Bike paths along roads end at every side street and resume across it, or
// jog onto the carriageway and back, so the route makes two turns a few metres apart that together keep the rider
// going the same way: "turn left", "turn right" where the rider rides straight on. Two turns within kJogMeters of
// each other, joined by a road without a name (a bike path, a crossing) or back onto the road left, are merged:
// dropped if the rider comes out within kStraightDegrees of the way they came in, else one turn in the combined
// direction.
double constexpr kJogMeters = 45;
double constexpr kStraightDegrees = 30;

void MergeJogs(std::vector<routing::RouteSegment> & segments);

// Bicycle directions: the car ones (upstream uses them for bicycles too), then MergeJogs.
class BicycleDirectionsEngine : public routing::CarDirectionsEngine
{
public:
  using routing::CarDirectionsEngine::CarDirectionsEngine;

protected:
  void FixupTurns(std::vector<routing::RouteSegment> & segments) override;
};
}  // namespace grove
