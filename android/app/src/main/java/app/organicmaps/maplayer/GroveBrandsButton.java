package app.organicmaps.maplayer;

import android.content.res.ColorStateList;
import androidx.annotation.NonNull;
import app.organicmaps.R;
import app.organicmaps.sdk.GroveBrands;
import app.organicmaps.util.ThemeUtils;
import com.google.android.material.floatingactionbutton.FloatingActionButton;

// Grove: the top-left map button, upstream's help and donation button, shows and hides chains' logos. Help and
// donating stay in the main menu.
final class GroveBrandsButton
{
  private GroveBrandsButton() {}

  static void toggle()
  {
    GroveBrands.nativeSetShown(!GroveBrands.nativeAreShown());
  }

  // Accent when logos show, the plain button colour when they don't. Always true: the button is Grove's.
  static boolean update(@NonNull FloatingActionButton button)
  {
    final boolean shown = GroveBrands.nativeAreShown();
    button.setImageResource(R.drawable.ic_grove_brands);
    button.setContentDescription(button.getContext().getString(R.string.grove_brand_logos));
    button.setImageTintList(ColorStateList.valueOf(
        ThemeUtils.getColor(button.getContext(), shown ? androidx.appcompat.R.attr.colorAccent : R.attr.iconTint)));
    return true;
  }
}
