#include "testing/testing.hpp"

#include "modules/elevation/map/elevation.hpp"

namespace grove_elevation_tests
{
UNIT_TEST(GroveElevation_Terrarium)
{
  TEST_EQUAL(grove::TerrariumMeters(128, 0, 0), 0.0, ());
  TEST_EQUAL(grove::TerrariumMeters(128, 23, 128), 23.5, ());
  TEST_EQUAL(grove::TerrariumMeters(127, 250, 0), -6.0, ());  // Below sea level, as in the Netherlands.
}

UNIT_TEST(GroveElevation_TilePixel)
{
  // Amsterdam's Dam square at zoom 10: tile 525/336, as OpenStreetMap numbers it.
  auto const t = grove::ToTilePixel({52.3731, 4.8926}, 10);
  TEST_EQUAL(t.m_x, 525, ());
  TEST_EQUAL(t.m_y, 336, ());
  TEST_LESS(t.m_px, 256, ());
  TEST_LESS(t.m_py, 256, ());

  // The world's corners stay inside the tile grid.
  auto const nw = grove::ToTilePixel({89.9, -180}, 3);
  TEST_EQUAL(nw.m_x, 0, ());
  TEST_EQUAL(nw.m_y, 0, ());
  auto const se = grove::ToTilePixel({-89.9, 180}, 3);
  TEST_EQUAL(se.m_x, 7, ());
  TEST_EQUAL(se.m_y, 7, ());
}
}  // namespace grove_elevation_tests

#include "modules/offline_layers/map/offline_layers.hpp"

#include "geometry/mercator.hpp"

namespace grove_offline_layers_tests
{
UNIT_TEST(GroveOfflineLayers_TileRange)
{
  // Around Amsterdam's Dam square: its zoom 10 tile is 525/336, as for the elevation.
  auto const p = mercator::FromLatLon(52.3731, 4.8926);
  auto const range = grove::OfflineLayers::ToTileRange({p, p}, 10);
  TEST_EQUAL(range.m_x0, 525, ());
  TEST_EQUAL(range.m_x1, 525, ());
  TEST_EQUAL(range.m_y0, 336, ());
  TEST_EQUAL(range.m_y1, 336, ());

  // The Netherlands at zoom 7 spans a few tiles, north row first.
  auto const nl =
      grove::OfflineLayers::ToTileRange({mercator::FromLatLon(50.75, 3.35), mercator::FromLatLon(53.55, 7.25)}, 7);
  TEST_EQUAL(nl.m_x0, 65, ());
  TEST_EQUAL(nl.m_x1, 66, ());
  TEST_EQUAL(nl.m_y0, 41, ());
  TEST_EQUAL(nl.m_y1, 42, ());
}
}  // namespace grove_offline_layers_tests
