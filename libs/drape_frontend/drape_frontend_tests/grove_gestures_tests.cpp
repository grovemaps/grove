#include "testing/testing.hpp"

#include "drape_frontend/grove_gestures.hpp"

namespace grove_gestures_tests
{
using grove::ClassifyTwoFingers;
using grove::TwoFingerGesture;

UNIT_TEST(GroveGestures_Classify)
{
  m2::PointD const a(100, 500), b(300, 500);
  double const threshold = 24;

  // Too little movement to tell.
  TEST_EQUAL(ClassifyTwoFingers(a, b, a + m2::PointD(0, -10), b + m2::PointD(0, -10), threshold),
             TwoFingerGesture::Undecided, ());
  // Both fingers up together: tilt.
  TEST_EQUAL(ClassifyTwoFingers(a, b, a + m2::PointD(3, -60), b + m2::PointD(-2, -58), threshold),
             TwoFingerGesture::Tilt, ());
  TEST_EQUAL(ClassifyTwoFingers(a, b, a + m2::PointD(0, 60), b + m2::PointD(0, 60), threshold), TwoFingerGesture::Tilt,
             ());
  // Pinch, spread, sideways drag and opposite vertical moves (rotation): scale.
  TEST_EQUAL(ClassifyTwoFingers(a, b, a + m2::PointD(50, 0), b + m2::PointD(-50, 0), threshold),
             TwoFingerGesture::Scale, ());
  TEST_EQUAL(ClassifyTwoFingers(a, b, a + m2::PointD(0, -60), b + m2::PointD(0, 60), threshold),
             TwoFingerGesture::Scale, ());
  TEST_EQUAL(ClassifyTwoFingers(a, b, a + m2::PointD(60, 0), b + m2::PointD(60, 0), threshold), TwoFingerGesture::Scale,
             ());
  // Up together but spreading too: scale.
  TEST_EQUAL(ClassifyTwoFingers(a, b, a + m2::PointD(0, -60), m2::PointD(420, 440), threshold), TwoFingerGesture::Scale,
             ());
}

UNIT_TEST(GroveGestures_TiltAngle)
{
  double const max = 0.95;
  // Up half the screen tilts fully; down flattens; never beyond the limits.
  TEST_ALMOST_EQUAL_ABS(grove::TiltAngle(0, -500, 1000, max), max, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(grove::TiltAngle(0, -250, 1000, max), max / 2, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(grove::TiltAngle(max, 1000, 1000, max), 0.0, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(grove::TiltAngle(0.5, -2000, 1000, max), max, 1e-9, ());
}
}  // namespace grove_gestures_tests
