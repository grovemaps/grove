#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "app/organicmaps/sdk/Framework.hpp"

#include "modules/reviews/map/reviews.hpp"

#include <ctime>

// Grove: Mangrove reviews of the selected place, see modules/reviews/map/reviews.hpp.
namespace
{
jobject ToJavaResult(JNIEnv * env, grove::reviews::PlaceReviews const & reviews)
{
  static jclass const reviewClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/GroveReviews$Review");
  static jmethodID const reviewCtor =
      jni::GetConstructorID(env, reviewClass, "(ILjava/lang/String;Ljava/lang/String;J)V");
  static jclass const resultClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/GroveReviews$Result");
  static jmethodID const resultCtor =
      jni::GetConstructorID(env, resultClass, "([Lapp/organicmaps/sdk/GroveReviews$Review;FFIZ)V");

  jni::TScopedLocalObjectArrayRef array(
      env, jni::ToJavaArray(env, reviewClass, reviews.m_reviews, [](JNIEnv * env, grove::reviews::Review const & r)
  {
    jni::TScopedLocalRef opinion(env, jni::ToJavaString(env, r.m_opinion));
    jni::TScopedLocalRef author(env, jni::ToJavaString(env, r.m_author));
    return env->NewObject(reviewClass, reviewCtor, static_cast<jint>(r.m_rating), opinion.get(), author.get(),
                          static_cast<jlong>(r.m_time));
  }));
  auto const trend = reviews.RecentTrend(std::time(nullptr));
  return env->NewObject(resultClass, resultCtor, array.get(), static_cast<jfloat>(reviews.AverageStars()),
                        static_cast<jfloat>(trend ? trend->m_recentStars : 0),
                        static_cast<jint>(trend ? trend->m_recentCount : 0),
                        static_cast<jboolean>(trend && trend->m_better));
}
}  // namespace

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

JNIEXPORT jobject Java_app_organicmaps_sdk_GroveReviews_nativeGetBundled(JNIEnv * env, jclass, jstring subjectUri)
{
  auto const subject = grove::reviews::FromUri(jni::ToNativeString(env, subjectUri));
  return ToJavaResult(env, subject ? grove::reviews::FindBundled(*subject) : grove::reviews::PlaceReviews{});
}

JNIEXPORT jobject Java_app_organicmaps_sdk_GroveReviews_nativeFetch(JNIEnv * env, jclass, jstring subjectUri)
{
  auto const subject = grove::reviews::FromUri(jni::ToNativeString(env, subjectUri));
  if (!subject)
    return nullptr;
  auto online = grove::reviews::Fetch(*subject);
  if (!online)
    return nullptr;
  online->Merge(grove::reviews::FindBundled(*subject));
  return ToJavaResult(env, *online);
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
