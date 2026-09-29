#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "modules/measure_tap/map/measure.hpp"

#include <memory>

// Grove: the two-finger tap distance, see modules/measure_tap/map/measure.hpp.
extern "C"
{
JNIEXPORT void Java_app_organicmaps_sdk_GroveMeasure_nativeSetListener(JNIEnv * env, jclass, jobject listener)
{
  if (listener == nullptr)
  {
    grove::SetMeasureListener({});
    return;
  }

  // Deleted with the last copy of the function, on the UI thread.
  std::shared_ptr<_jobject> const ref(env->NewGlobalRef(listener),
                                      [](jobject obj) { jni::GetEnv()->DeleteGlobalRef(obj); });
  grove::SetMeasureListener([ref](float x1, float y1, float x2, float y2, std::string const & distance)
  {
    JNIEnv * env = jni::GetEnv();
    static jmethodID const method = jni::GetMethodID(env, ref.get(), "onDistance", "(FFFFLjava/lang/String;)V");
    jni::TScopedLocalRef text(env, jni::ToJavaString(env, distance));
    env->CallVoidMethod(ref.get(), method, x1, y1, x2, y2, text.get());
  });
}
}
