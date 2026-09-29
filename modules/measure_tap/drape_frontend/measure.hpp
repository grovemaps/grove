#pragma once

#include "geometry/point2d.hpp"

#include <functional>

namespace grove
{
// A two-finger tap shows the distance between the fingers, as in Guru Maps (upstream zooms out). FrontendRenderer
// hands the fingers' screen and map points to this, which modules/measure_tap/map/measure.hpp sets before the engine
// starts; not set, the tap zooms out as upstream's.
using MeasureFn = std::function<void(m2::PointD const & pixel1, m2::PointD const & pixel2, m2::PointD const & point1,
                                     m2::PointD const & point2)>;

inline MeasureFn & GetMeasureFn()
{
  static MeasureFn fn;
  return fn;
}
}  // namespace grove
