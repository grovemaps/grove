package app.organicmaps.widget.placepage;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import app.organicmaps.R;
import app.organicmaps.sdk.GroveFeatures;
import app.organicmaps.sdk.GroveTrips;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.bookmarks.data.Track;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.util.UiUtils;

// Grove: a route saved as a track keeps its stops; this row plans it again to navigate it. See
// RoutingManager::GroveTripOfTrack.
final class GroveTripRow
{
  private GroveTripRow() {}

  // The row ends the card's preview; it joins upstream's layout when a saved trip first shows.
  static void update(@NonNull ViewGroup preview, @NonNull MapObject mapObject)
  {
    View row = preview.findViewById(R.id.grove_trip_navigate);
    final boolean on = GroveFeatures.isOn("saved_trips");
    final int router = on && mapObject.isTrack() ? GroveTrips.nativeGetRouter(((Track) mapObject).getTrackId()) : -1;
    if (row == null && router >= 0)
      row = LayoutInflater.from(preview.getContext())
                .inflate(R.layout.grove_trip_row, preview, true)
                .findViewById(R.id.grove_trip_navigate);
    if (row == null)
      return;
    UiUtils.showIf(router >= 0, row);
    if (router < 0)
      return;
    final long trackId = ((Track) mapObject).getTrackId();
    row.setOnClickListener(v -> {
      if (!GroveTrips.nativeRestore(trackId))
        return;
      final RoutingController controller = RoutingController.get();
      if (controller.isPlanning() || controller.isNavigating())
        controller.cancel();
      controller.setRouterType(Router.valueOf(router));
      controller.restoreRoute();
    });
  }
}
