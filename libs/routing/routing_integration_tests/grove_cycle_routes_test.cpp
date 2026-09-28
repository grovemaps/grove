#include "testing/testing.hpp"

#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "routing/grove_cycle_routes.hpp"

#include "geometry/mercator.hpp"

#include "base/scope_guard.hpp"

namespace grove_cycle_routes_test
{
using namespace integration;
using namespace routing;

struct Routes
{
  double m_preferM = 0, m_preferS = 0;
  double m_plainM = 0, m_plainS = 0;
};

// The route with and without the cycle route preference.
Routes CalcBoth(ms::LatLon const & from, ms::LatLon const & to)
{
  bool const saved = grove::PreferCycleRoutes();
  SCOPE_GUARD(restore, [saved] { grove::SetPreferCycleRoutes(saved); });

  Routes routes;
  for (bool prefer : {true, false})
  {
    grove::SetPreferCycleRoutes(prefer);
    auto const [route, code] = CalculateRoute(GetVehicleComponents(VehicleType::Bicycle), mercator::FromLatLon(from),
                                              {0., 0.}, mercator::FromLatLon(to));
    TEST_EQUAL(code, RouterResultCode::NoError, ());
    (prefer ? routes.m_preferM : routes.m_plainM) = route->GetTotalDistanceMeters();
    (prefer ? routes.m_preferS : routes.m_plainS) = route->GetTotalTimeSec();
  }
  // A cycle route is worth a small detour only.
  TEST_LESS_OR_EQUAL(routes.m_preferS, routes.m_plainS * 1.1, ());
  return routes;
}

UNIT_TEST(Grove_CycleRouteFactor)
{
  TEST_EQUAL(grove::CycleRouteFactor("ncn"), 1.2, ());
  TEST_EQUAL(grove::CycleRouteFactor("rcn"), 1.2, ());
  TEST_EQUAL(grove::CycleRouteFactor("lcn"), 1.1, ());
}

UNIT_TEST(Grove_Netherlands_Utrecht_CycleRoutes)
{
  // Maarssen to Utrecht Science Park: takes the regional routes, a slightly different way.
  auto const routes = CalcBoth({52.1300, 5.0400}, {52.0700, 5.1500});
  TEST_NOT_EQUAL(routes.m_preferM, routes.m_plainM, ());

  // De Meern to the centre: a few metres longer at most.
  CalcBoth({52.0806, 5.0335}, {52.0894, 5.1100});
}

UNIT_TEST(Grove_Netherlands_Amsterdam_NoLongDetour)
{
  // Across the centre: the regional routes around it are far longer, so they are not taken.
  auto const routes = CalcBoth({52.3400, 4.8500}, {52.3900, 4.9300});
  TEST_EQUAL(routes.m_preferM, routes.m_plainM, ());
}
}  // namespace grove_cycle_routes_test
