package app.organicmaps.widget.placepage;

import android.os.Handler;
import android.os.Looper;
import android.widget.TextView;
import androidx.annotation.NonNull;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.GroveElevation;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.util.UiUtils;
import java.util.Locale;

// Grove: the height of the place card's point, next to its coordinates. See libs/map/grove_elevation.hpp.
final class GrovePlaceElevation
{
  private static final Handler sMainHandler = new Handler(Looper.getMainLooper());

  private GrovePlaceElevation() {}

  static void show(@NonNull TextView view, double lat, double lon)
  {
    // About 1 m: the card refreshes its coordinates as the user's own position moves.
    final String key = String.format(Locale.ROOT, "%.5f,%.5f", lat, lon);
    if (key.equals(view.getTag()))
      return;
    view.setTag(key);
    UiUtils.hide(view);
    ThreadPool.getWorker().execute(() -> {
      final double meters = GroveElevation.nativeGet(lat, lon);
      sMainHandler.post(() -> {
        // Another point may have been shown meanwhile.
        if (!key.equals(view.getTag()) || Double.isNaN(meters))
          return;
        view.setText("▲ " + Framework.nativeFormatAltitude(meters));
        UiUtils.show(view);
      });
    });
  }
}
