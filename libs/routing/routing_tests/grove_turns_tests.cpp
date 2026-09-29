#include "testing/testing.hpp"

#include "routing/grove_turns.hpp"

#include "geometry/mercator.hpp"

#include <string>
#include <vector>

namespace grove_turns_tests
{
using namespace routing;
using turns::CarDirection;

struct Point
{
  double m_lat, m_lon;
  CarDirection m_turn = CarDirection::None;  // At this point.
  std::string m_road;                        // Of the piece leading here.
};

std::vector<RouteSegment> MakeRoute(std::vector<Point> const & points)
{
  std::vector<RouteSegment> segments;
  for (auto const & p : points)
    segments.emplace_back(Segment(), turns::TurnItem(0, p.m_turn),
                          geometry::PointWithAltitude(mercator::FromLatLon(p.m_lat, p.m_lon), 0),
                          RouteSegment::RoadNameInfo(p.m_road));
  return segments;
}

std::vector<CarDirection> Turns(std::vector<RouteSegment> const & segments)
{
  std::vector<CarDirection> turns;
  for (auto const & s : segments)
    if (!s.GetTurn().IsTurnNone())
      turns.push_back(s.GetTurn().m_turn);
  return turns;
}

// Riding east along latitude 52; 0.0002 degrees of longitude is about 14 m, of latitude about 22 m.
double constexpr kLat = 52.0;

UNIT_TEST(GroveTurns_BikePathJogIsStraight)
{
  // Left onto a crossing without a name, right back onto the bike path 11 m later: straight on.
  auto route = MakeRoute({{kLat, 5.0000, CarDirection::None, "Street"},
                          {kLat, 5.0002},
                          {kLat, 5.0004, CarDirection::TurnLeft},
                          {kLat + 0.0001, 5.0004, CarDirection::TurnRight},
                          {kLat + 0.0001, 5.0006},
                          {kLat + 0.0001, 5.0008},
                          {kLat + 0.0001, 5.0010, CarDirection::ReachedYourDestination}});
  grove::MergeJogs(route, grove::JogMode::Bicycle);
  TEST_EQUAL(Turns(route), std::vector<CarDirection>{CarDirection::ReachedYourDestination}, ());
}

UNIT_TEST(GroveTurns_TwoNamedStreetsStay)
{
  // Left into one street, right into the next 33 m on: both are real.
  auto route = MakeRoute({{kLat, 5.0000, CarDirection::None, "A"},
                          {kLat, 5.0002, CarDirection::None, "A"},
                          {kLat, 5.0004, CarDirection::TurnLeft, "A"},
                          {kLat + 0.0003, 5.0004, CarDirection::TurnRight, "B"},
                          {kLat + 0.0003, 5.0006, CarDirection::None, "C"},
                          {kLat + 0.0003, 5.0008, CarDirection::None, "C"},
                          {kLat + 0.0003, 5.0010, CarDirection::ReachedYourDestination, "C"}});
  grove::MergeJogs(route, grove::JogMode::Bicycle);
  TEST_EQUAL(Turns(route),
             std::vector<CarDirection>(
                 {CarDirection::TurnLeft, CarDirection::TurnRight, CarDirection::ReachedYourDestination}),
             ());
}

UNIT_TEST(GroveTurns_ShortNamedCornerIsAJog)
{
  // Right onto a street for 11 m, then left onto the bike path along it: one left-ish turn at most, not two.
  auto route = MakeRoute({{kLat, 5.0000, CarDirection::None, "A"},
                          {kLat, 5.0002, CarDirection::None, "A"},
                          {kLat, 5.0004, CarDirection::TurnRight, "A"},
                          {kLat - 0.0001, 5.0004, CarDirection::TurnLeft, "B"},
                          {kLat - 0.0001, 5.0006},
                          {kLat - 0.0001, 5.0008},
                          {kLat - 0.0001, 5.0010, CarDirection::ReachedYourDestination}});
  grove::MergeJogs(route, grove::JogMode::Bicycle);
  TEST_EQUAL(Turns(route), std::vector<CarDirection>{CarDirection::ReachedYourDestination}, ());
}

UNIT_TEST(GroveTurns_TwoLeftsBecomeOne)
{
  // Two lefts 11 m apart on a bike path: one sharp left, where the rider ends up heading back.
  auto route = MakeRoute({{kLat, 5.0000},
                          {kLat, 5.0002},
                          {kLat, 5.0004, CarDirection::TurnLeft},
                          {kLat + 0.0001, 5.0004, CarDirection::TurnLeft},
                          {kLat + 0.0001, 5.0002},
                          {kLat + 0.0001, 5.0000},
                          {kLat + 0.0001, 4.9998, CarDirection::ReachedYourDestination}});
  grove::MergeJogs(route, grove::JogMode::Bicycle);
  auto const turns = Turns(route);
  TEST_EQUAL(turns.size(), 2, (turns));
  TEST(turns[0] == CarDirection::TurnSharpLeft || turns[0] == CarDirection::TurnLeft, (turns));
}

UNIT_TEST(GroveTurns_BikeRunOfThreeTurns)
{
  // Left, right and left again within 30 m around a traffic island, coming out heading north: one left turn.
  auto route = MakeRoute({{kLat, 5.0000, CarDirection::None, "Street"},
                          {kLat, 5.0002},
                          {kLat, 5.0004, CarDirection::TurnLeft},
                          {kLat + 0.0001, 5.0004, CarDirection::TurnRight},
                          {kLat + 0.0001, 5.0005, CarDirection::TurnLeft},
                          {kLat + 0.0003, 5.0005},
                          {kLat + 0.0005, 5.0005},
                          {kLat + 0.0007, 5.0005, CarDirection::ReachedYourDestination}});
  grove::MergeJogs(route, grove::JogMode::Bicycle);
  auto const turns = Turns(route);
  TEST_EQUAL(turns.size(), 2, (turns));
  TEST(turns::IsLeftTurn(turns[0]), (turns));
}

UNIT_TEST(GroveTurns_CarTurnsIntoTwoStreetsStay)
{
  // Left into one street, right into the next 11 m on: a car really makes both turns, even without names.
  auto route = MakeRoute({{kLat, 5.0000, CarDirection::None, "A"},
                          {kLat, 5.0002, CarDirection::None, "A"},
                          {kLat, 5.0004, CarDirection::TurnLeft, "A"},
                          {kLat + 0.0001, 5.0004, CarDirection::TurnRight},
                          {kLat + 0.0001, 5.0006},
                          {kLat + 0.0001, 5.0008},
                          {kLat + 0.0001, 5.0010, CarDirection::ReachedYourDestination}});
  grove::MergeJogs(route, grove::JogMode::Car);
  TEST_EQUAL(Turns(route),
             std::vector<CarDirection>(
                 {CarDirection::TurnLeft, CarDirection::TurnRight, CarDirection::ReachedYourDestination}),
             ());
}

UNIT_TEST(GroveTurns_CarWiggleOnOneRoad)
{
  // Slight left and right 11 m apart, staying on the same street: nothing to say.
  auto route = MakeRoute({{kLat, 5.0000, CarDirection::None, "A"},
                          {kLat, 5.0002, CarDirection::None, "A"},
                          {kLat, 5.0004, CarDirection::TurnSlightLeft, "A"},
                          {kLat + 0.00005, 5.0005, CarDirection::TurnSlightRight, "A"},
                          {kLat + 0.00005, 5.0007, CarDirection::None, "A"},
                          {kLat + 0.00005, 5.0009, CarDirection::None, "A"},
                          {kLat + 0.00005, 5.0011, CarDirection::ReachedYourDestination, "A"}});
  grove::MergeJogs(route, grove::JogMode::Car);
  TEST_EQUAL(Turns(route), std::vector<CarDirection>{CarDirection::ReachedYourDestination}, ());
}

std::vector<RouteSegment> MakeWalk(std::vector<std::pair<Point, turns::PedestrianDirection>> const & points)
{
  std::vector<RouteSegment> segments;
  for (auto const & [p, dir] : points)
    segments.emplace_back(Segment(), turns::TurnItem(0, dir),
                          geometry::PointWithAltitude(mercator::FromLatLon(p.m_lat, p.m_lon), 0),
                          RouteSegment::RoadNameInfo(p.m_road));
  return segments;
}

std::vector<turns::PedestrianDirection> WalkTurns(std::vector<RouteSegment> const & segments)
{
  std::vector<turns::PedestrianDirection> turns;
  for (auto const & s : segments)
    if (!s.GetTurn().IsTurnNone())
      turns.push_back(s.GetTurn().m_pedestrianTurn);
  return turns;
}

UNIT_TEST(GroveTurns_WalkAcrossASquare)
{
  using turns::PedestrianDirection;
  // Right, left, left and right along the edges of a square named alike, coming out heading east as before: straight
  // on.
  auto route = MakeWalk({{{kLat, 5.0000, CarDirection::None, "Platz"}, PedestrianDirection::None},
                         {{kLat, 5.0002, CarDirection::None, "Platz"}, PedestrianDirection::None},
                         {{kLat, 5.0004, CarDirection::None, "Platz"}, PedestrianDirection::TurnRight},
                         {{kLat - 0.0001, 5.0004, CarDirection::None, "Platz"}, PedestrianDirection::TurnLeft},
                         {{kLat - 0.0001, 5.0006, CarDirection::None, "Platz"}, PedestrianDirection::TurnLeft},
                         {{kLat, 5.0006, CarDirection::None, "Platz"}, PedestrianDirection::TurnRight},
                         {{kLat, 5.0008, CarDirection::None, "Platz"}, PedestrianDirection::None},
                         {{kLat, 5.0010, CarDirection::None, "Platz"}, PedestrianDirection::None},
                         {{kLat, 5.0012, CarDirection::None, "Platz"}, PedestrianDirection::ReachedYourDestination}});
  grove::MergeJogs(route, grove::JogMode::Pedestrian);
  TEST_EQUAL(WalkTurns(route), std::vector<PedestrianDirection>{PedestrianDirection::ReachedYourDestination}, ());
}

UNIT_TEST(GroveTurns_WalkCrossingThenTurn)
{
  using turns::PedestrianDirection;
  // Left onto a crossing without a name, a right-hand bend 11 m on, and on north: one left turn.
  auto route = MakeWalk({{{kLat, 5.0000, CarDirection::None, "A"}, PedestrianDirection::None},
                         {{kLat, 5.0002, CarDirection::None, "A"}, PedestrianDirection::None},
                         {{kLat, 5.0004, CarDirection::None, "A"}, PedestrianDirection::TurnLeft},
                         {{kLat + 0.0001, 5.0004}, PedestrianDirection::TurnRight},
                         {{kLat + 0.0003, 5.0004}, PedestrianDirection::None},
                         {{kLat + 0.0005, 5.0004}, PedestrianDirection::None},
                         {{kLat + 0.0007, 5.0004}, PedestrianDirection::ReachedYourDestination}});
  grove::MergeJogs(route, grove::JogMode::Pedestrian);
  TEST_EQUAL(
      WalkTurns(route),
      std::vector<PedestrianDirection>({PedestrianDirection::TurnLeft, PedestrianDirection::ReachedYourDestination}),
      ());
}
}  // namespace grove_turns_tests
