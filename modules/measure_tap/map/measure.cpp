#include "modules/measure_tap/map/measure.hpp"

#include "modules/measure_tap/drape_frontend/measure.hpp"

#include "platform/distance.hpp"
#include "platform/platform.hpp"

#include "geometry/mercator.hpp"

#include <memory>
#include <mutex>

namespace grove
{
namespace
{
std::mutex g_mutex;
std::shared_ptr<MeasureListener> g_listener;
}  // namespace

void InitMeasure()
{
  GetMeasureFn() =
      [](m2::PointD const & pixel1, m2::PointD const & pixel2, m2::PointD const & point1, m2::PointD const & point2)
  {
    std::string distance = platform::Distance::CreateFormatted(mercator::DistanceOnEarth(point1, point2)).ToString();
    GetPlatform().RunTask(Platform::Thread::Gui, [pixel1, pixel2, distance = std::move(distance)]
    {
      std::shared_ptr<MeasureListener> listener;
      {
        std::lock_guard lock(g_mutex);
        listener = g_listener;
      }
      if (listener && *listener)
        (*listener)(pixel1.x, pixel1.y, pixel2.x, pixel2.y, distance);
    });
  };
}

void SetMeasureListener(MeasureListener listener)
{
  std::lock_guard lock(g_mutex);
  g_listener = std::make_shared<MeasureListener>(std::move(listener));
}
}  // namespace grove
