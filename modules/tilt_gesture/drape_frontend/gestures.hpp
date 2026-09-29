#pragma once

#include "geometry/point2d.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace grove
{
// Two-finger gestures after Guru and Google Maps (see UserEventStream): both fingers sliding up or down together tilt
// the map into 3D; spreading, pinching or turning them zooms and rotates as before.
enum class TwoFingerGesture
{
  Undecided,
  Scale,
  Tilt,
};

inline std::string DebugPrint(TwoFingerGesture gesture)
{
  switch (gesture)
  {
  case TwoFingerGesture::Undecided: return "Undecided";
  case TwoFingerGesture::Scale: return "Scale";
  case TwoFingerGesture::Tilt: return "Tilt";
  }
  return {};
}

// Decides from where the fingers started and are now (pixels), once one of them has moved threshold pixels.
inline TwoFingerGesture ClassifyTwoFingers(m2::PointD const & start1, m2::PointD const & start2,
                                           m2::PointD const & now1, m2::PointD const & now2, double threshold)
{
  m2::PointD const d1 = now1 - start1;
  m2::PointD const d2 = now2 - start2;
  if (std::max(d1.Length(), d2.Length()) < threshold)
    return TwoFingerGesture::Undecided;

  bool const vertical = std::abs(d1.y) > 2 * std::abs(d1.x) && std::abs(d2.y) > 2 * std::abs(d2.x);
  bool const together = d1.y * d2.y > 0;
  double const startSpread = start1.Length(start2);
  bool const keepsSpread = startSpread > 0 && std::abs(now1.Length(now2) - startSpread) < 0.15 * startSpread;
  return vertical && together && keepsSpread ? TwoFingerGesture::Tilt : TwoFingerGesture::Scale;
}

// The tilt after the fingers moved dy pixels (negative: up). Sliding up the whole screen height tilts twice the
// maximum, so half a screen is enough.
inline double TiltAngle(double startAngle, double dy, double screenHeight, double maxAngle)
{
  return std::clamp(startAngle - dy / screenHeight * 2 * maxAngle, 0.0, maxAngle);
}
}  // namespace grove
