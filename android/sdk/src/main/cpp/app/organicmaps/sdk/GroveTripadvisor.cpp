#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "app/organicmaps/sdk/Framework.hpp"

#include "map/grove_tripadvisor.hpp"

#include "platform/preferred_languages.hpp"

#include <ctime>

// Grove: Tripadvisor ratings with the user's own API key, see libs/map/grove_tripadvisor.hpp.
extern "C"
{
JNIEXPORT jstring Java_app_organicmaps_sdk_GroveTripadvisor_nativeGetKey(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, grove::tripadvisor::GetKey());
}

JNIEXPORT void Java_app_organicmaps_sdk_GroveTripadvisor_nativeSetKey(JNIEnv * env, jclass, jstring key)
{
  grove::tripadvisor::SetKey(jni::ToNativeString(env, key));
}

JNIEXPORT jstring Java_app_organicmaps_sdk_GroveTripadvisor_nativeGetSelectedSubject(JNIEnv * env, jclass)
{
  auto * f = frm();
  if (grove::tripadvisor::GetKey().empty() || !f->HasPlacePageInfo())
    return nullptr;
  auto const subject = grove::reviews::GetSubject(f->GetCurrentPlacePageInfo());
  return subject && !subject->m_name.empty() ? jni::ToJavaString(env, grove::reviews::ToUri(*subject)) : nullptr;
}

JNIEXPORT jobject Java_app_organicmaps_sdk_GroveTripadvisor_nativeFetch(JNIEnv * env, jclass, jstring subjectUri)
{
  auto const subject = grove::reviews::FromUri(jni::ToNativeString(env, subjectUri));
  if (!subject)
    return nullptr;
  auto const ratings = grove::tripadvisor::Fetch(*subject, languages::GetCurrentNorm());
  if (!ratings)
    return nullptr;

  static jclass const reviewClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/GroveReviews$Review");
  static jmethodID const reviewCtor =
      jni::GetConstructorID(env, reviewClass, "(ILjava/lang/String;Ljava/lang/String;J)V");
  static jclass const resultClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/GroveTripadvisor$Result");
  static jmethodID const resultCtor = jni::GetConstructorID(
      env, resultClass, "(Ljava/lang/String;Ljava/lang/String;FI[Lapp/organicmaps/sdk/GroveReviews$Review;FIZ)V");

  jni::TScopedLocalObjectArrayRef reviews(env, jni::ToJavaArray(env, reviewClass, ratings->m_reviews.m_reviews,
                                                                [](JNIEnv * env, grove::reviews::Review const & r)
  {
    jni::TScopedLocalRef opinion(env, jni::ToJavaString(env, r.m_opinion));
    jni::TScopedLocalRef author(env, jni::ToJavaString(env, r.m_author));
    return env->NewObject(reviewClass, reviewCtor, static_cast<jint>(r.m_rating), opinion.get(), author.get(),
                          static_cast<jlong>(r.m_time));
  }));
  jni::TScopedLocalRef name(env, jni::ToJavaString(env, ratings->m_name));
  jni::TScopedLocalRef url(env, jni::ToJavaString(env, ratings->m_url));
  auto const trend = ratings->RecentTrend(std::time(nullptr));
  return env->NewObject(
      resultClass, resultCtor, name.get(), url.get(), static_cast<jfloat>(ratings->m_stars),
      static_cast<jint>(ratings->m_count), reviews.get(), static_cast<jfloat>(trend ? trend->m_recentStars : 0),
      static_cast<jint>(trend ? trend->m_recentCount : 0), static_cast<jboolean>(trend && trend->m_better));
}
}
