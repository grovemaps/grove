#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "platform/settings.hpp"

// Grove: the map's look, see libs/indexer/map_style_reader.cpp.
extern "C"
{
JNIEXPORT jstring Java_app_organicmaps_sdk_GroveLook_nativeGet(JNIEnv * env, jclass)
{
  std::string look = "grove";
  settings::TryGet("GroveLook", look);
  return jni::ToJavaString(env, look);
}

JNIEXPORT void Java_app_organicmaps_sdk_GroveLook_nativeSet(JNIEnv * env, jclass, jstring look)
{
  settings::Set("GroveLook", jni::ToNativeString(env, look));
}

JNIEXPORT jboolean Java_app_organicmaps_sdk_GroveLook_nativeGetNavigationColors(JNIEnv *, jclass)
{
  bool grove = false;
  settings::TryGet("GroveNavigationColors", grove);
  return static_cast<jboolean>(grove);
}

JNIEXPORT void Java_app_organicmaps_sdk_GroveLook_nativeSetNavigationColors(JNIEnv *, jclass, jboolean grove)
{
  settings::Set("GroveNavigationColors", static_cast<bool>(grove));
}
}
