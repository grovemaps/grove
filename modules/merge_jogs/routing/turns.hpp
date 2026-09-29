#pragma once

#include "routing/car_directions.hpp"
#include "routing/pedestrian_directions.hpp"
#include "routing/route.hpp"

#include <vector>

namespace grove
{
// Cleans up turn instructions. Bike paths along roads end at every side street and resume across it, or jog onto
// the carriageway and back; walking routes cross squares and streets along the edges of the map's footway graph;
// roads wiggle at junctions. So routes make runs of turns a few metres apart that together keep the traveller going
// the same way: "turn left", "turn right" where they go straight on. Turns within a mode's jog distance of each other
// are merged when the pieces between are jogs (see JogMode): dropped if the traveller comes out within
// kStraightDegrees of the way they came in, else one turn in the combined direction.
double constexpr kStraightDegrees = 30;

enum class JogMode
{
  // Only back onto the road left, as a car may really turn into one street and at once out of it.
  Car,
  // Also pieces without a name (bike paths, crossings) and pieces too short to be a street (the corner of one crossed
  // to reach a bike path).
  Bicycle,
  Pedestrian,
};

// Turns closer than this are merged.
double JogMeters(JogMode mode);

void MergeJogs(std::vector<routing::RouteSegment> & segments, JogMode mode);

// Car and bicycle directions: upstream's, then MergeJogs.
class CarDirectionsEngine : public routing::CarDirectionsEngine
{
public:
  CarDirectionsEngine(routing::MwmDataSource & dataSource, std::shared_ptr<routing::NumMwmIds> numMwmIds, JogMode mode)
    : routing::CarDirectionsEngine(dataSource, std::move(numMwmIds))
    , m_mode(mode)
  {}

protected:
  void FixupTurns(std::vector<routing::RouteSegment> & segments) override;

private:
  JogMode m_mode;
};

class PedestrianDirectionsEngine : public routing::PedestrianDirectionsEngine
{
public:
  using routing::PedestrianDirectionsEngine::PedestrianDirectionsEngine;

protected:
  void FixupTurns(std::vector<routing::RouteSegment> & segments) override;
};
}  // namespace grove
