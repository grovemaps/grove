#include <jni.h>

#include "platform/grove_performance.hpp"

// Grove: the performance boost switch, see libs/platform/grove_performance.hpp.
extern "C"
{
JNIEXPORT jboolean Java_app_organicmaps_sdk_GrovePerformance_nativeIsEnabled(JNIEnv *, jclass)
{
  return static_cast<jboolean>(grove::SavedPerformanceBoost());
}

JNIEXPORT void Java_app_organicmaps_sdk_GrovePerformance_nativeSetEnabled(JNIEnv *, jclass, jboolean enabled)
{
  grove::SetPerformanceBoost(static_cast<bool>(enabled));
}
}
