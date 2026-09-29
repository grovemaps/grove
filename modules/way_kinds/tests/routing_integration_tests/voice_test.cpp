#include "testing/testing.hpp"

#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "modules/way_kinds/routing/way_kind.hpp"

#include "geometry/mercator.hpp"

namespace grove_voice_test
{
using namespace integration;
using namespace routing;

// The distance from each turn to the one before, arrival aside.
std::vector<double> TurnGaps(Route const & route)
{
  std::vector<double> gaps;
  double last = 0;
  for (auto const & s : route.GetRouteSegments())
  {
    if (s.GetTurn().IsTurnNone() || s.GetTurn().IsTurnReachedYourDestination())
      continue;
    gaps.push_back(s.GetDistFromBeginningMeters() - last);
    last = s.GetDistFromBeginningMeters();
  }
  return gaps;
}

UNIT_TEST(Grove_Netherlands_Amsterdam_OntoTheBikePath)
{
  // Centraal to the Vondelpark: "onto the bike path" where the route leaves a street for one, not again at the turns
  // along bike paths.
  auto const [route, code] =
      CalculateRoute(GetVehicleComponents(VehicleType::Bicycle), mercator::FromLatLon(52.3791, 4.9003), {0., 0.},
                     mercator::FromLatLon(52.3580, 4.8686));
  TEST_EQUAL(code, RouterResultCode::NoError, ());
  auto const & segments = route->GetRouteSegments();
  size_t ontoBikePath = 0;
  for (size_t i = 0; i + 1 < segments.size(); ++i)
  {
    if (segments[i].GetTurn().IsTurnNone())
      continue;
    // A segment's turn is at its end, where the next one starts.
    RouteSegment::RoadNameInfo next;
    route->GetClosestStreetNameAfterIdx(i + 1, next);
    if (next.m_groveWayKind != grove::WayKind::BikePath)
      continue;
    ++ontoBikePath;
    TEST(!grove::IsSameWayKind(segments[i].GetRoadNameInfo().m_groveWayKind, grove::WayKind::BikePath), (i));
  }
  TEST_GREATER_OR_EQUAL(ontoBikePath, 3, ());
}

UNIT_TEST(Grove_Netherlands_Amsterdam_WalkWithoutJogs)
{
  // The Dam to the Anne Frank House: crossings and pavements made turns 5-22 m apart; 13 turns became 5.
  auto const [route, code] =
      CalculateRoute(GetVehicleComponents(VehicleType::Pedestrian), mercator::FromLatLon(52.3731, 4.8926), {0., 0.},
                     mercator::FromLatLon(52.3752, 4.8840));
  TEST_EQUAL(code, RouterResultCode::NoError, ());
  auto const gaps = TurnGaps(*route);
  TEST_LESS_OR_EQUAL(gaps.size(), 7, (gaps));
  for (double gap : gaps)
    TEST_GREATER(gap, 15.0, (gaps));
}

UNIT_TEST(Grove_Austria_Seefeld_CarWiggle)
{
  // Münchner Straße wiggles at a junction: "keep left", "keep left", "turn right" within 25 m became one "turn left".
  auto const [route, code] =
      CalculateRoute(GetVehicleComponents(VehicleType::Car), mercator::FromLatLon(47.3240, 11.1850), {0., 0.},
                     mercator::FromLatLon(47.3297, 11.1878));
  TEST_EQUAL(code, RouterResultCode::NoError, ());
  for (double gap : TurnGaps(*route))
    TEST_GREATER(gap, 20.0, (TurnGaps(*route)));
}
}  // namespace grove_voice_test
