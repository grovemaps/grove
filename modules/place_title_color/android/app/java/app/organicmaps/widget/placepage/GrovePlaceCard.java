package app.organicmaps.widget.placepage;

import android.content.Context;
import android.graphics.Color;
import android.widget.TextView;
import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.core.graphics.ColorUtils;
import app.organicmaps.sdk.GroveFeatures;
import app.organicmaps.sdk.GrovePlace;
import app.organicmaps.util.ThemeUtils;

// Grove: the place card's title takes the colour of the place, its chain's logo colour or its category colour,
// darkened or lightened until it reads.
final class GrovePlaceCard
{
  private GrovePlaceCard() {}

  static void apply(@NonNull TextView title)
  {
    if (!GroveFeatures.isOn("place_title_color"))
      return;
    final Context context = title.getContext();
    final int rgb = GrovePlace.nativeGetSelectedColor();
    if (rgb == 0)
      title.setTextColor(ThemeUtils.getColor(context, android.R.attr.textColorPrimary));
    else
      title.setTextColor(readable(Color.BLACK | rgb, ThemeUtils.isDarkTheme(context)));
  }

  @ColorInt
  private static int readable(@ColorInt int color, boolean dark)
  {
    for (int i = 0; i < 8; ++i)
    {
      final double luminance = ColorUtils.calculateLuminance(color);
      if (dark ? luminance >= 0.45 : luminance <= 0.3)
        break;
      color = ColorUtils.blendARGB(color, dark ? Color.WHITE : Color.BLACK, 0.15f);
    }
    return color;
  }
}
