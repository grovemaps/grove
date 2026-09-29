#include <jni.h>

#include "modules/core/platform/features.hpp"

// Grove: the performance boost switch, see modules/performance_boost/platform/performance.hpp.
extern "C"
{
JNIEXPORT jboolean Java_app_organicmaps_sdk_GrovePerformance_nativeIsEnabled(JNIEnv *, jclass)
{
  return static_cast<jboolean>(grove::IsSwitchedOn(grove::Feature::PerformanceBoost));
}

JNIEXPORT void Java_app_organicmaps_sdk_GrovePerformance_nativeSetEnabled(JNIEnv *, jclass, jboolean enabled)
{
  grove::SetSwitch(grove::Feature::PerformanceBoost, enabled);
}
}
