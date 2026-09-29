package app.organicmaps.search;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.LayerDrawable;
import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.content.res.AppCompatResources;
import app.organicmaps.util.ThemeUtils;
import java.util.HashMap;
import java.util.Map;

// Grove: search categories in the map icons' colors (data/styles/grove/icon-colors.txt), so the list matches the
// map. Upstream's category icons are a glyph on a colored circle; only the circle is recolored.
final class GroveCategoryIcons
{
  private GroveCategoryIcons() {}

  // Light and dark colors by label category, as icon-colors.txt has them.
  private static final int[] FOOD = {0xFFFC8A30, 0xFFCF7630};
  private static final int[] SHOP = {0xFFFBBF1C, 0xFFCE9F20};
  private static final int[] ENTERTAINMENT = {0xFFC4488E, 0xFFA14077};
  private static final int[] CULTURE = {0xFF5E76CC, 0xFF5264A7};
  private static final int[] HOTEL = {0xFF9256D6, 0xFF7B4CAF};
  private static final int[] SPORT = {0xFF26C2D8, 0xFF26A0B1};
  private static final int[] WATER = {0xFF2CC8EA, 0xFF2CA5C0};
  private static final int[] NATURE = {0xFF34B852, 0xFF309747};
  private static final int[] NEUTRAL = {0xFF8E8E9A, 0xFF75757E};
  private static final int[] HEALTH = {0xFFE8456F, 0xFFBE3F60};
  private static final int[] TRANSPORT = {0xFF5CA4F6, 0xFF528ACA};

  private static final Map<String, int[]> COLORS = new HashMap<>();
  static
  {
    COLORS.put("category_eat", FOOD);
    COLORS.put("category_food", FOOD);
    COLORS.put("category_shopping", SHOP);
    COLORS.put("category_secondhand", SHOP);
    COLORS.put("category_entertainment", ENTERTAINMENT);
    COLORS.put("category_nightlife", ENTERTAINMENT);
    COLORS.put("category_tourism", CULTURE);
    COLORS.put("category_hotel", HOTEL);
    COLORS.put("category_children", SPORT);
    COLORS.put("category_water", WATER);
    COLORS.put("category_recycling", NATURE);
    COLORS.put("category_hospital", HEALTH);
    COLORS.put("category_pharmacy", HEALTH);
    COLORS.put("category_transport", TRANSPORT);
    COLORS.put("category_bus", TRANSPORT);
    COLORS.put("category_tram", TRANSPORT);
    COLORS.put("category_parking", TRANSPORT);
    COLORS.put("category_fuel", TRANSPORT);
    COLORS.put("category_rv", TRANSPORT);
    COLORS.put("category_atm", NEUTRAL);
    COLORS.put("category_bank", NEUTRAL);
    COLORS.put("category_police", NEUTRAL);
    COLORS.put("category_post", NEUTRAL);
    COLORS.put("category_toilet", NEUTRAL);
    COLORS.put("category_wifi", NEUTRAL);
    COLORS.put("category_luggagehero", NEUTRAL);
  }

  @Nullable
  static Drawable get(@NonNull Context context, @NonNull String key, @DrawableRes int iconResId)
  {
    final Drawable icon = AppCompatResources.getDrawable(context, iconResId);
    final int[] colors = COLORS.get(key);
    if (icon == null || colors == null)
      return icon;
    final Drawable mutated = icon.mutate();
    if (mutated instanceof LayerDrawable layers && layers.getNumberOfLayers() > 0
        && layers.getDrawable(0) instanceof GradientDrawable circle)
      circle.setColor(colors[ThemeUtils.isDarkTheme(context) ? 1 : 0]);
    return mutated;
  }
}
