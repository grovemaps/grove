#include "routing/grove_turns.hpp"

#include "routing/turns.hpp"
#include "routing/turns_generator_utils.hpp"

#include "geometry/mercator.hpp"

#include "base/math.hpp"

#include <cmath>

namespace grove
{
namespace
{
using routing::turns::CarDirection;

// Directions for the way ahead only: not roundabouts, arrival or lane hints.
bool IsManeuver(CarDirection d)
{
  return routing::turns::IsLeftOrRightTurn(d) || d == CarDirection::GoStraight || d == CarDirection::UTurnLeft ||
         d == CarDirection::UTurnRight;
}

// A route point at least this far before or after a turn gives its direction, as the turn generator's do.
double constexpr kBearingMeters = 12;
// Shorter pieces between two turns are jogs even when named: the corner of a street crossed to reach a bike path.
double constexpr kShortPieceMeters = 15;
// Coming out nearly the way the rider came in.
double constexpr kReverseDegrees = 150;

m2::PointD Point(std::vector<routing::RouteSegment> const & segments, size_t i)
{
  return segments[i].GetJunction().GetPoint();
}
}  // namespace

void MergeJogs(std::vector<routing::RouteSegment> & segments)
{
  // A segment's turn is at its end, route point i, and its name is the road leading there.
  for (size_t i = 0; i < segments.size(); ++i)
  {
    if (!IsManeuver(segments[i].GetTurn().m_turn))
      continue;
    // The next turn, if within kJogMeters along the route.
    size_t j = i;
    double meters = 0;
    do
    {
      ++j;
      if (j < segments.size())
        meters += mercator::DistanceOnEarth(Point(segments, j - 1), Point(segments, j));
    }
    while (j < segments.size() && segments[j].GetTurn().IsTurnNone() && meters <= kJogMeters);
    if (j + 1 >= segments.size() || !IsManeuver(segments[j].GetTurn().m_turn) || meters > kJogMeters)
      continue;

    // Only jogs: a bike path or crossing without a name between the turns, a piece too short to be a street, or
    // back onto the same road. Two turns into two named streets are both real.
    bool unnamed = true;
    for (size_t k = i + 1; k <= j && unnamed; ++k)
      unnamed = segments[k].GetRoadNameInfo().m_name.empty();
    auto const & roadIn = segments[i].GetRoadNameInfo().m_name;
    bool const sameRoad = !roadIn.empty() && roadIn == segments[j + 1].GetRoadNameInfo().m_name;
    if (!unnamed && !sameRoad && meters >= kShortPieceMeters)
      continue;

    m2::PointD const turnIn = Point(segments, i);
    size_t from = i;
    while (from > 0 && mercator::DistanceOnEarth(Point(segments, from), turnIn) < kBearingMeters)
      --from;
    m2::PointD const turnOut = Point(segments, j);
    size_t to = j;
    while (to + 1 < segments.size() && mercator::DistanceOnEarth(Point(segments, to), turnOut) < kBearingMeters)
      ++to;
    if (from == i || to == j)
      continue;

    // The turn from the way in to the way out, as if both met at one point.
    m2::PointD const inDir = turnIn - Point(segments, from);
    m2::PointD const outDir = Point(segments, to) - turnOut;
    double const angle = math::RadToDeg(routing::turns::PiMinusTwoVectorsAngle(m2::PointD::Zero(), -inDir, outDir));

    if (std::abs(angle) < kStraightDegrees)
      segments[i].ClearTurn();
    else if (std::abs(angle) > kReverseDegrees)  // Which side is ambiguous: the first turn's.
      segments[i].SetTurnDirection(routing::turns::IsLeftTurn(segments[i].GetTurn().m_turn)
                                       ? CarDirection::TurnSharpLeft
                                       : CarDirection::TurnSharpRight);
    else
      segments[i].SetTurnDirection(routing::turns::IntermediateDirection(angle));
    segments[j].ClearTurn();
    i = j;
  }
}

void BicycleDirectionsEngine::FixupTurns(std::vector<routing::RouteSegment> & segments)
{
  routing::CarDirectionsEngine::FixupTurns(segments);
  MergeJogs(segments);
}
}  // namespace grove
