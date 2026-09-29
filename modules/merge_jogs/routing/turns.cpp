#include "modules/merge_jogs/routing/turns.hpp"

#include "routing/turns.hpp"
#include "routing/turns_generator_utils.hpp"

#include "geometry/mercator.hpp"

#include "base/math.hpp"

#include <cmath>
#include <optional>
#include <string>
#include <utility>

namespace grove
{
namespace
{
using routing::RouteSegment;
using routing::turns::CarDirection;
using routing::turns::PedestrianDirection;

// A route point at least this far before or after a turn gives its direction, as the turn generator's do.
double constexpr kBearingMeters = 12;
// Shorter pieces between two turns are jogs even when named: the corner of a street crossed to reach a bike path.
double constexpr kShortPieceMeters = 15;
// Coming out nearly the way the traveller came in.
double constexpr kReverseDegrees = 150;

// Directions for the way ahead only: not roundabouts, exits, arrival or lane hints.
bool IsManeuver(RouteSegment const & s, JogMode mode)
{
  auto const & turn = s.GetTurn();
  if (mode == JogMode::Pedestrian)
    return turn.m_pedestrianTurn == PedestrianDirection::TurnLeft ||
           turn.m_pedestrianTurn == PedestrianDirection::TurnRight ||
           turn.m_pedestrianTurn == PedestrianDirection::GoStraight;
  CarDirection const d = turn.m_turn;
  return routing::turns::IsLeftOrRightTurn(d) || d == CarDirection::GoStraight || d == CarDirection::UTurnLeft ||
         d == CarDirection::UTurnRight;
}

bool IsLeft(RouteSegment const & s, JogMode mode)
{
  return mode == JogMode::Pedestrian ? s.GetTurn().m_pedestrianTurn == PedestrianDirection::TurnLeft
                                     : routing::turns::IsLeftTurn(s.GetTurn().m_turn);
}

m2::PointD Point(std::vector<RouteSegment> const & segments, size_t i)
{
  return segments[i].GetJunction().GetPoint();
}

std::string const & Name(std::vector<RouteSegment> const & segments, size_t i)
{
  return segments[i].GetRoadNameInfo().m_name;
}

// The next turn after the one at i, and the metres to it, if within maxMeters along the route.
std::optional<std::pair<size_t, double>> NextTurn(std::vector<RouteSegment> const & segments, size_t i,
                                                  double maxMeters)
{
  double meters = 0;
  for (size_t j = i + 1; j < segments.size(); ++j)
  {
    meters += mercator::DistanceOnEarth(Point(segments, j - 1), Point(segments, j));
    if (meters > maxMeters)
      return {};
    if (!segments[j].GetTurn().IsTurnNone())
      return std::make_pair(j, meters);
  }
  return {};
}

// Sets the merged turn at i, from the way in to the way out of the turns at i and last.
void SetMerged(std::vector<RouteSegment> & segments, size_t i, size_t last, JogMode mode)
{
  m2::PointD const turnIn = Point(segments, i);
  size_t from = i;
  while (from > 0 && mercator::DistanceOnEarth(Point(segments, from), turnIn) < kBearingMeters)
    --from;
  m2::PointD const turnOut = Point(segments, last);
  size_t to = last;
  while (to + 1 < segments.size() && mercator::DistanceOnEarth(Point(segments, to), turnOut) < kBearingMeters)
    ++to;
  if (from == i || to == last)
    return;

  // The turn from the way in to the way out, as if both met at one point.
  m2::PointD const inDir = turnIn - Point(segments, from);
  m2::PointD const outDir = Point(segments, to) - turnOut;
  double const angle = math::RadToDeg(routing::turns::PiMinusTwoVectorsAngle(m2::PointD::Zero(), -inDir, outDir));

  bool const pedestrian = mode == JogMode::Pedestrian;
  if (std::abs(angle) < kStraightDegrees)
  {
    segments[i].ClearTurn();
  }
  else if (std::abs(angle) > kReverseDegrees)
  {
    // Which side is ambiguous: the first turn's.
    bool const left = IsLeft(segments[i], mode);
    if (pedestrian)
      segments[i].SetPedestrianTurn(left ? PedestrianDirection::TurnLeft : PedestrianDirection::TurnRight);
    else if (mode == JogMode::Car)
      segments[i].SetTurnDirection(left ? CarDirection::UTurnLeft : CarDirection::UTurnRight);
    else
      segments[i].SetTurnDirection(left ? CarDirection::TurnSharpLeft : CarDirection::TurnSharpRight);
  }
  else if (pedestrian)
  {
    segments[i].SetPedestrianTurn(routing::turns::IntermediateDirectionPedestrian(angle));
  }
  else
  {
    segments[i].SetTurnDirection(routing::turns::IntermediateDirection(angle));
  }
  for (size_t k = i + 1; k <= last; ++k)
    segments[k].ClearTurn();
}
}  // namespace

double JogMeters(JogMode mode)
{
  switch (mode)
  {
  case JogMode::Car: return 30;
  case JogMode::Bicycle: return 45;
  case JogMode::Pedestrian: return 40;
  }
  UNREACHABLE();
}

void MergeJogs(std::vector<RouteSegment> & segments, JogMode mode)
{
  double const jogMeters = JogMeters(mode);
  // A segment's turn is at its end, route point i, and its name is the road leading there.
  for (size_t i = 0; i < segments.size(); ++i)
  {
    if (!IsManeuver(segments[i], mode))
      continue;

    // The run of turns that are jogs, over at most 1.5 jog distances, lest a run of real turns merges into one.
    std::string const & roadIn = Name(segments, i);
    size_t last = i;
    double span = 0;
    while (auto const next = NextTurn(segments, last, jogMeters))
    {
      auto const [j, meters] = *next;
      if (j + 1 >= segments.size() || !IsManeuver(segments[j], mode) || span + meters > 1.5 * jogMeters)
        break;
      bool unnamed = mode != JogMode::Car;
      for (size_t k = last + 1; k <= j && unnamed; ++k)
        unnamed = Name(segments, k).empty();
      bool const sameRoad = !roadIn.empty() && roadIn == Name(segments, j + 1);
      bool const shortPiece = meters < kShortPieceMeters && mode != JogMode::Car;
      if (!unnamed && !sameRoad && !shortPiece)
        break;
      last = j;
      span += meters;
    }
    if (last == i)
      continue;

    SetMerged(segments, i, last, mode);
    i = last;
  }
}

void CarDirectionsEngine::FixupTurns(std::vector<RouteSegment> & segments)
{
  routing::CarDirectionsEngine::FixupTurns(segments);
  MergeJogs(segments, m_mode);
}

void PedestrianDirectionsEngine::FixupTurns(std::vector<RouteSegment> & segments)
{
  routing::PedestrianDirectionsEngine::FixupTurns(segments);
  MergeJogs(segments, JogMode::Pedestrian);
}
}  // namespace grove
