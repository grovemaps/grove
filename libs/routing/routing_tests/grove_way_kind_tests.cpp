#include "testing/testing.hpp"

#include "routing/grove_way_kind.hpp"
#include "routing/turns_sound_settings.hpp"
#include "routing/turns_tts_text.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature_data.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace grove_way_kind_tests
{
using namespace routing;
using namespace routing::turns;
using namespace routing::turns::sound;
using grove::WayKind;

WayKind Kind(std::vector<std::vector<std::string_view>> const & paths)
{
  feature::TypesHolder types;
  for (auto const & path : paths)
    types.Add(classif().GetTypeByPath(path));
  return grove::GetWayKind(types);
}

UNIT_TEST(GroveWayKind_Types)
{
  classificator::Load();
  TEST_EQUAL(Kind({{"highway", "cycleway"}}), WayKind::BikePath, ());
  TEST_EQUAL(Kind({{"highway", "cycleway", "bridge"}}), WayKind::BikePath, ());
  TEST_EQUAL(Kind({{"highway", "footway", "bicycle"}}), WayKind::SharedPath, ());
  TEST_EQUAL(Kind({{"highway", "path", "bicycle"}}), WayKind::SharedPath, ());
  // A bike path along a footway, drawn as one way.
  TEST_EQUAL(Kind({{"highway", "footway"}, {"highway", "cycleway"}}), WayKind::BikePath, ());
  TEST_EQUAL(Kind({{"highway", "footway"}}), WayKind::Path, ());
  TEST_EQUAL(Kind({{"highway", "steps"}}), WayKind::Steps, ());
  TEST_EQUAL(Kind({{"highway", "track", "bridge"}}), WayKind::Track, ());
  // Parts of their streets.
  TEST_EQUAL(Kind({{"highway", "footway", "crossing"}}), WayKind::None, ());
  TEST_EQUAL(Kind({{"highway", "footway", "sidewalk"}}), WayKind::None, ());
  TEST_EQUAL(Kind({{"highway", "residential"}}), WayKind::None, ());
  TEST_EQUAL(Kind({{"highway", "pedestrian"}}), WayKind::None, ());

  TEST(grove::IsSameWayKind(WayKind::BikePath, WayKind::SharedPath), ());
  TEST(grove::IsSameWayKind(WayKind::SharedPath, WayKind::Path), ());
  TEST(!grove::IsSameWayKind(WayKind::BikePath, WayKind::Path), ());
  TEST(!grove::IsSameWayKind(WayKind::None, WayKind::BikePath), ());
  TEST(!grove::IsSameWayKind(WayKind::SharedPath, WayKind::Track), ());
}

UNIT_TEST(GroveWayKind_Voice)
{
  std::string const json = R"({
      "in_200_meters":"In 200 meters.",
      "onto":"onto",
      "make_a_right_turn":"Make a right turn.",
      "make_a_left_turn":"Make a left turn.",
      "make_a_right_turn_street":"NULL",
      "make_a_left_turn_street":"NULL",
      "dist_direction_onto_street":"%1$s %2$s %3$s %4$s",
      "grove_the_bike_path":"the bike path",
      "grove_the_path":"the path",
      "grove_the_stairs":"NULL"
      })";
  GetTtsText tts;
  tts.ForTestingSetLocaleWithJson(json, "en");

  auto const way = [](WayKind kind, std::string name = {})
  {
    RouteSegment::RoadNameInfo info(std::move(name));
    info.m_groveWayKind = kind;
    return info;
  };
  auto const units = measurement_utils::Units::Metric;

  TEST_EQUAL(
      tts.GetTurnNotification(Notification(200, 0, false, CarDirection::TurnRight, units, way(WayKind::BikePath))),
      "In 200 meters Make a right turn onto the bike path", ());
  // At the turn itself too.
  TEST_EQUAL(tts.GetTurnNotification(Notification(0, 0, false, CarDirection::TurnRight, units, way(WayKind::BikePath))),
             "Make a right turn onto the bike path", ());
  // A name wins.
  TEST_EQUAL(tts.GetTurnNotification(
                 Notification(200, 0, false, CarDirection::TurnLeft, units, way(WayKind::BikePath, "Singel"))),
             "In 200 meters Make a left turn onto Singel", ());
  // A language without the phrase says the turn only.
  TEST_EQUAL(tts.GetTurnNotification(Notification(200, 0, false, CarDirection::TurnLeft, units, way(WayKind::Steps))),
             "In 200 meters. Make a left turn.", ());

  // A path for walking and cycling is a path to a walker.
  Notification walk(200, 0, false, CarDirection::None, units, way(WayKind::SharedPath));
  walk.m_turnDirPedestrian = PedestrianDirection::TurnLeft;
  TEST_EQUAL(tts.GetTurnNotification(walk), "In 200 meters Make a left turn onto the path", ());
}
}  // namespace grove_way_kind_tests
