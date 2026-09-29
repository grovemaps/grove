package app.organicmaps.sdk;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.WorkerThread;

// Grove: a place's Tripadvisor rating and newest reviews, with the user's own API key. See
// modules/tripadvisor/map/tripadvisor.hpp.
public final class GroveTripadvisor
{
  private GroveTripadvisor() {}

  // Created by JNI.
  @Keep
  public static final class Result
  {
    @NonNull
    public final String name;
    @NonNull
    public final String url; // The place's Tripadvisor page; the card must link to it.
    public final float stars; // 1..5, 0 without a rating.
    public final int count;
    @NonNull
    public final GroveReviews.Review[] reviews; // Newest first, at most 5.
    public final float recentStars; // The last half year's, 0 when not half a star or more from the rating.
    public final int recentCount;
    public final boolean better;

    @Keep
    Result(@NonNull String name, @NonNull String url, float stars, int count, @NonNull GroveReviews.Review[] reviews,
           float recentStars, int recentCount, boolean better)
    {
      this.name = name;
      this.url = url;
      this.stars = stars;
      this.count = count;
      this.reviews = reviews;
      this.recentStars = recentStars;
      this.recentCount = recentCount;
      this.better = better;
    }
  }

  @NonNull
  public static native String nativeGetKey();

  public static native void nativeSetKey(@NonNull String key);

  // The selected place, as a Mangrove subject, when it gets a Tripadvisor section: with a key, for named places people
  // review.
  @Nullable
  public static native String nativeGetSelectedSubject();

  // The place of a subject on Tripadvisor, or null without a key,
  // a match or on network errors. Blocks.
  @WorkerThread
  @Nullable
  public static native Result nativeFetch(@NonNull String subject);
}
