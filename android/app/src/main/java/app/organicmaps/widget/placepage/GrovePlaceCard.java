package app.organicmaps.widget.placepage;

import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.core.graphics.ColorUtils;
import androidx.fragment.app.Fragment;
import app.organicmaps.R;
import app.organicmaps.sdk.GrovePlace;
import app.organicmaps.util.ThemeUtils;
import com.google.android.material.shape.MaterialShapeDrawable;
import java.lang.ref.WeakReference;

// Grove: the place card takes the colour of the place, its chain's logo colour or its category colour: a light tint
// over the whole card, every section included, and the full colour for the title (darkened or lightened until it
// reads).
final class GrovePlaceCard
{
  private static final float CARD_TINT = 0.14f;

  // The place's colour (0: none) and the card it was applied to; sections that appear later (loaded fragments,
  // details shown when the card is pulled up) are tinted when the card lays them out.
  @ColorInt
  private static int sColor;
  private static WeakReference<View> sCard = new WeakReference<>(null);

  private GrovePlaceCard() {}

  static void apply(@NonNull Fragment fragment, @NonNull TextView title)
  {
    final Context context = fragment.requireContext();
    final int rgb = GrovePlace.nativeGetSelectedColor();
    sColor = rgb == 0 ? 0 : Color.BLACK | rgb;

    if (sColor == 0)
      title.setTextColor(ThemeUtils.getColor(context, android.R.attr.textColorPrimary));
    else
      title.setTextColor(readable(sColor, ThemeUtils.isDarkTheme(context)));

    final View card = fragment.requireActivity().findViewById(R.id.placepage);
    if (card == null)
      return;
    if (sCard.get() != card)
    {
      sCard = new WeakReference<>(card);
      card.getViewTreeObserver().addOnGlobalLayoutListener(() -> tint(card));
    }
    tint(card);
  }

  // Tints every view painted in the card or section-gap colour.
  private static void tint(@NonNull View card)
  {
    final Context context = card.getContext();
    final int cards = ContextCompat.getColor(context, R.color.bg_cards);
    final int panel = ContextCompat.getColor(context, R.color.bg_panel);
    tint(card, cards, panel);
  }

  private static void tint(@NonNull View view, @ColorInt int cards, @ColorInt int panel)
  {
    final int base = baseColor(view.getBackground());
    if (base != 0 && (base == cards || base == panel))
    {
      // Only on changes: this runs on every layout of the card.
      final int wanted = sColor == 0 ? 0 : ColorUtils.blendARGB(base, sColor, CARD_TINT);
      final ColorStateList current = view.getBackgroundTintList();
      if ((current == null ? 0 : current.getDefaultColor()) != wanted)
        view.setBackgroundTintList(wanted == 0 ? null : ColorStateList.valueOf(wanted));
    }
    if (view instanceof ViewGroup group)
    {
      for (int i = 0; i < group.getChildCount(); ++i)
        tint(group.getChildAt(i), cards, panel);
    }
  }

  // The colour a background paints, without tint: sections have plain colours, the card's top is the bottom sheet's
  // rounded shape.
  @ColorInt
  private static int baseColor(@Nullable Drawable background)
  {
    if (background instanceof ColorDrawable color)
      return color.getColor();
    if (background instanceof MaterialShapeDrawable shape && shape.getFillColor() != null)
      return shape.getFillColor().getDefaultColor();
    return 0;
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
