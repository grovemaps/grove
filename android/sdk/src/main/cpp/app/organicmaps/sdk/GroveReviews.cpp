#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "app/organicmaps/sdk/Framework.hpp"

#include "map/grove_reviews.hpp"

// Grove: Mangrove reviews of the selected place, see libs/map/grove_reviews.hpp.
extern "C"
{
JNIEXPORT jstring Java_app_organicmaps_sdk_GroveReviews_nativeGetSelectedSubject(JNIEnv * env, jclass)
{
  auto * f = frm();
  if (!grove::reviews::IsEnabled() || !f->HasPlacePageInfo())
    return nullptr;
  auto const subject = grove::reviews::GetSubject(f->GetCurrentPlacePageInfo());
  return subject ? jni::ToJavaString(env, grove::reviews::ToUri(*subject)) : nullptr;
}

JNIEXPORT jobjectArray Java_app_organicmaps_sdk_GroveReviews_nativeFetch(JNIEnv * env, jclass, jstring subjectUri)
{
  auto const subject = grove::reviews::FromUri(jni::ToNativeString(env, subjectUri));
  if (!subject)
    return nullptr;
  auto const reviews = grove::reviews::Fetch(*subject);
  if (!reviews)
    return nullptr;

  static jclass const reviewClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/GroveReviews$Review");
  static jmethodID const ctor = jni::GetConstructorID(env, reviewClass, "(ILjava/lang/String;Ljava/lang/String;J)V");
  return jni::ToJavaArray(env, reviewClass, reviews->m_reviews, [](JNIEnv * env, grove::reviews::Review const & r)
  {
    jni::TScopedLocalRef opinion(env, jni::ToJavaString(env, r.m_opinion));
    jni::TScopedLocalRef author(env, jni::ToJavaString(env, r.m_author));
    return env->NewObject(reviewClass, ctor, static_cast<jint>(r.m_rating), opinion.get(), author.get(),
                          static_cast<jlong>(r.m_time));
  });
}

JNIEXPORT jstring Java_app_organicmaps_sdk_GroveReviews_nativeGetWriteReviewUrl(JNIEnv * env, jclass,
                                                                                jstring subjectUri)
{
  auto const subject = grove::reviews::FromUri(jni::ToNativeString(env, subjectUri));
  return subject ? jni::ToJavaString(env, grove::reviews::WriteReviewUrl(*subject)) : nullptr;
}

JNIEXPORT jboolean Java_app_organicmaps_sdk_GroveReviews_nativeIsEnabled(JNIEnv *, jclass)
{
  return grove::reviews::IsEnabled();
}

JNIEXPORT void Java_app_organicmaps_sdk_GroveReviews_nativeSetEnabled(JNIEnv *, jclass, jboolean enabled)
{
  grove::reviews::SetEnabled(enabled);
}
}
