package app.organicmaps.widget.placepage;

import android.view.View;
import androidx.annotation.NonNull;
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

  static void update(@NonNull View row, @NonNull MapObject mapObject)
  {
    final int router = mapObject.isTrack() ? GroveTrips.nativeGetRouter(((Track) mapObject).getTrackId()) : -1;
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
