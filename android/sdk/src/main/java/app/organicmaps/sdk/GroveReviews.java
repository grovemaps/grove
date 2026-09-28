package app.organicmaps.sdk;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.WorkerThread;

// Grove: reviews of places from Mangrove (https://mangrove.reviews), see libs/map/grove_reviews.hpp.
public final class GroveReviews
{
  private GroveReviews() {}

  // Created by JNI.
  @Keep
  public static final class Review
  {
    public final int rating; // 0..100, 0 for reviews without a rating.
    @NonNull
    public final String opinion;
    @NonNull
    public final String author;
    public final long time; // Seconds since 1970.

    @Keep
    Review(int rating, @NonNull String opinion, @NonNull String author, long time)
    {
      this.rating = rating;
      this.opinion = opinion;
      this.author = author;
      this.time = time;
    }

    // 1..5 stars.
    public int getStars()
    {
      return 1 + Math.round(rating / 25f);
    }
  }

  // The selected place's Mangrove subject, or null for places people don't review or when reviews are switched off.
  @Nullable
  public static native String nativeGetSelectedSubject();

  // The place's reviews, newest first, or null on network errors. Blocks until Mangrove answers.
  @WorkerThread
  @Nullable
  public static native Review[] nativeFetch(@NonNull String subject);

  // Mangrove's web page for writing a review of the place.
  @Nullable
  public static native String nativeGetWriteReviewUrl(@NonNull String subject);

  public static native boolean nativeIsEnabled();

  public static native void nativeSetEnabled(boolean enabled);
}
