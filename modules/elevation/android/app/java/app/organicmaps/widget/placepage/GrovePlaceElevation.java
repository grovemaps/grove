package app.organicmaps.widget.placepage;

import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.core.widget.TextViewCompat;
import app.organicmaps.R;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.GroveElevation;
import app.organicmaps.sdk.GroveFeatures;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.util.UiUtils;
import java.util.Locale;

// Grove: the height of the place card's point, next to its coordinates. See modules/elevation/map/elevation.hpp.
final class GrovePlaceElevation
{
  private static final Handler sMainHandler = new Handler(Looper.getMainLooper());

  private GrovePlaceElevation() {}

  // Next to the coordinates, in upstream's row (ll__place_latlon), which the height's view joins when first shown.
  static void show(@NonNull View frame, double lat, double lon)
  {
    final LinearLayout row = frame.findViewById(R.id.ll__place_latlon);
    TextView view = row.findViewById(R.id.tv__place_elevation);
    if (!GroveFeatures.isOn("elevation"))
    {
      if (view != null)
        UiUtils.hide(view);
      return;
    }
    if (view == null)
      view = addView(row);
    show(view, lat, lon);
  }

  @NonNull
  private static TextView addView(@NonNull LinearLayout row)
  {
    // The coordinates take the free space, the height sits at the end.
    final View latlon = row.findViewById(R.id.tv__place_latlon);
    latlon.setLayoutParams(new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
    final TextView view = new TextView(row.getContext());
    view.setId(R.id.tv__place_elevation);
    TextViewCompat.setTextAppearance(view, R.style.MwmTextAppearance_PlacePage);
    final LinearLayout.LayoutParams params =
        new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
    params.setMarginStart(row.getResources().getDimensionPixelSize(R.dimen.margin_half));
    UiUtils.hide(view);
    row.addView(view, params);
    return view;
  }

  private static void show(@NonNull TextView view, double lat, double lon)
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
