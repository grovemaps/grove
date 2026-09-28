package app.organicmaps.widget.placepage;

import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.Color;
import android.view.View;
import android.widget.TextView;
import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;
import androidx.core.graphics.ColorUtils;
import androidx.fragment.app.Fragment;
import app.organicmaps.R;
import app.organicmaps.sdk.GrovePlace;
import app.organicmaps.util.ThemeUtils;

// Grove: the place card takes the colour of the place, its chain's logo colour or its category colour: a light tint
// for the card, the full colour for the title (darkened or lightened until it reads on the card).
final class GrovePlaceCard
{
  private static final float CARD_TINT = 0.14f;

  private GrovePlaceCard() {}

  static void apply(@NonNull Fragment fragment, @NonNull TextView title)
  {
    final View card = fragment.requireActivity().findViewById(R.id.placepage);
    final Context context = fragment.requireContext();
    final int rgb = GrovePlace.nativeGetSelectedColor();
    if (rgb == 0)
    {
      if (card != null)
        card.setBackgroundTintList(null);
      title.setTextColor(ThemeUtils.getColor(context, android.R.attr.textColorPrimary));
      return;
    }

    final int color = Color.BLACK | rgb;
    final boolean dark = ThemeUtils.isDarkTheme(context);
    if (card != null)
    {
      final int base = ContextCompat.getColor(context, R.color.bg_cards);
      card.setBackgroundTintList(ColorStateList.valueOf(ColorUtils.blendARGB(base, color, CARD_TINT)));
    }
    title.setTextColor(readable(color, dark));
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
