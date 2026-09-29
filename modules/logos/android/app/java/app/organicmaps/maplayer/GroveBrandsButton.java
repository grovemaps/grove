package app.organicmaps.maplayer;

import android.content.res.ColorStateList;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.R;
import app.organicmaps.sdk.GroveBrands;
import app.organicmaps.sdk.GroveFeatures;
import app.organicmaps.util.ThemeUtils;
import com.google.android.material.floatingactionbutton.FloatingActionButton;

// Grove: the top-left map button, upstream's help and donation button, shows and hides chains' logos. Help and
// donating stay in the main menu. With the logos module off, the button is upstream's.
final class GroveBrandsButton
{
  private GroveBrandsButton() {}

  static void attach(@Nullable FloatingActionButton button, @NonNull Runnable updateIcon)
  {
    if (button == null || !GroveFeatures.isOn("logos"))
      return;
    button.setOnClickListener(v -> {
      GroveBrands.nativeSetShown(!GroveBrands.nativeAreShown());
      updateIcon.run();
    });
  }

  // Accent when logos show, the plain button colour when they don't. False when the button is upstream's.
  static boolean update(@NonNull FloatingActionButton button)
  {
    if (!GroveFeatures.isOn("logos"))
      return false;
    final boolean shown = GroveBrands.nativeAreShown();
    button.setImageResource(R.drawable.ic_grove_brands);
    button.setContentDescription(button.getContext().getString(R.string.grove_brand_logos));
    button.setImageTintList(ColorStateList.valueOf(
        ThemeUtils.getColor(button.getContext(), shown ? androidx.appcompat.R.attr.colorAccent : R.attr.iconTint)));
    return true;
  }
}
