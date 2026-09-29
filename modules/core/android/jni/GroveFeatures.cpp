#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "app/organicmaps/sdk/Framework.hpp"

#include "modules/core/platform/features.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"

#include <string>
#include <vector>

// Grove: the modules' switches for the app, see modules/core/platform/features.hpp.
namespace
{
grove::FeatureInfo const & Find(JNIEnv * env, jstring module)
{
  auto const name = jni::ToNativeString(env, module);
  for (auto const & info : grove::Features())
    if (info.m_module == name)
      return info;
  CHECK(false, ("Unknown Grove module", name));
  return grove::Features().front();
}
}  // namespace

extern "C"
{
JNIEXPORT jobjectArray Java_app_organicmaps_sdk_GroveFeatures_nativeModules(JNIEnv * env, jclass)
{
  std::vector<std::string> modules;
  for (auto const & info : grove::Features())
    modules.emplace_back(info.m_module);
  return jni::ToJavaStringArray(env, modules);
}

JNIEXPORT jobjectArray Java_app_organicmaps_sdk_GroveFeatures_nativeTexts(JNIEnv * env, jclass, jstring module)
{
  auto const & info = Find(env, module);
  std::vector<std::string> const texts = {std::string(grove::GroupName(info.m_group)),
                                          std::string(grove::GroupIcon(info.m_group)), std::string(info.m_icon),
                                          std::string(info.m_about)};
  return jni::ToJavaStringArray(env, texts);
}

JNIEXPORT jboolean Java_app_organicmaps_sdk_GroveFeatures_nativeNeedsRestart(JNIEnv * env, jclass, jstring module)
{
  return static_cast<jboolean>(Find(env, module).m_restart);
}

JNIEXPORT jboolean Java_app_organicmaps_sdk_GroveFeatures_nativeIsOn(JNIEnv * env, jclass, jstring module)
{
  return static_cast<jboolean>(grove::IsOn(Find(env, module).m_feature));
}

JNIEXPORT jboolean Java_app_organicmaps_sdk_GroveFeatures_nativeIsSwitchedOn(JNIEnv * env, jclass, jstring module)
{
  return static_cast<jboolean>(grove::IsSwitchedOn(Find(env, module).m_feature));
}

JNIEXPORT void Java_app_organicmaps_sdk_GroveFeatures_nativeSetSwitch(JNIEnv * env, jclass, jstring module, jboolean on)
{
  auto const & info = Find(env, module);
  grove::SetSwitch(info.m_feature, static_cast<bool>(on));
  // A live feature shows at once: the map re-reads its tiles, and raster layers request or drop theirs.
  if (!info.m_restart)
    frm()->InvalidateRect(mercator::Bounds::FullRect());
}

JNIEXPORT jboolean Java_app_organicmaps_sdk_GroveFeatures_nativeIsStock(JNIEnv *, jclass)
{
  return static_cast<jboolean>(grove::IsStock());
}

JNIEXPORT jboolean Java_app_organicmaps_sdk_GroveFeatures_nativeIsStockSaved(JNIEnv *, jclass)
{
  return static_cast<jboolean>(grove::IsStockSaved());
}

JNIEXPORT void Java_app_organicmaps_sdk_GroveFeatures_nativeSetStock(JNIEnv *, jclass, jboolean stock)
{
  grove::SetStock(static_cast<bool>(stock));
}
}
