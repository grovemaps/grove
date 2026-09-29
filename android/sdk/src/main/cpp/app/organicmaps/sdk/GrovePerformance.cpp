#include <jni.h>

#include "platform/grove_features.hpp"

// Grove: the performance boost switch, see libs/platform/grove_performance.hpp.
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
