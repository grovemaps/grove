package app.organicmaps.settings;

import android.content.Context;
import android.graphics.drawable.Animatable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.util.AttributeSet;
import android.widget.ImageView;
import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;
import androidx.vectordrawable.graphics.drawable.AnimatedVectorDrawableCompat;
import app.organicmaps.R;
import app.organicmaps.util.ThemeUtils;

// Grove: a settings section, drawn as a colored card with an animated illustration on the start page
// ("grove_section_<name>" keys), or as the illustration heading the section's own page ("grove_hero_<name>"). See
// GroveSections and res/xml/grove_prefs_sections.xml.
public class GroveSectionPreference extends Preference
{
  private final String mSection;
  private final boolean mHero;

  public GroveSectionPreference(@NonNull Context context, @Nullable AttributeSet attrs)
  {
    super(context, attrs);
    final String key = getKey() == null ? "" : getKey();
    mHero = key.startsWith("grove_hero_");
    mSection = key.substring(key.lastIndexOf('_') + 1);
    setLayoutResource(mHero ? R.layout.grove_settings_section_hero : R.layout.grove_settings_section_card);
  }

  @Override
  public void onBindViewHolder(@NonNull PreferenceViewHolder holder)
  {
    super.onBindViewHolder(holder);
    final Context context = getContext();
    final float density = context.getResources().getDisplayMetrics().density;
    final int[] colors = colors(mSection, ThemeUtils.isDarkTheme(context));
    final GradientDrawable background = new GradientDrawable(GradientDrawable.Orientation.TL_BR, colors);
    background.setCornerRadius((mHero ? 28 : 22) * density);
    holder.findViewById(R.id.grove_section_background).setBackground(background);

    final ImageView icon = (ImageView) holder.findViewById(R.id.grove_section_icon);
    final Drawable animation = AnimatedVectorDrawableCompat.create(context, illustration(mSection));
    icon.setImageDrawable(animation);
    if (animation instanceof Animatable animatable)
      animatable.start();
  }

  // Gradients in the map icons' category colors, darker in the dark theme.
  @NonNull
  private static int[] colors(@NonNull String section, boolean dark)
  {
    final int[] light = switch (section)
    {
      case "grove" -> new int[] {0xFF34B852, 0xFF26A0B1};
      case "map" -> new int[] {0xFF34B852, 0xFF2C8FD6};
      case "navigation" -> new int[] {0xFF3478F6, 0xFF5E5CE6};
      case "privacy" -> new int[] {0xFF2CA5A5, 0xFF34B852};
      case "app" -> new int[] {0xFF7C8594, 0xFF4F5663};
      default -> new int[] {0xFF9256D6, 0xFFC4488E};
    };
    if (!dark)
      return light;
    final int[] result = new int[light.length];
    for (int i = 0; i < light.length; ++i)
    {
      final int c = light[i];
      result[i] =
          0xFF000000 | ((((c >> 16) & 0xFF) * 3 / 4) << 16) | ((((c >> 8) & 0xFF) * 3 / 4) << 8) | ((c & 0xFF) * 3 / 4);
    }
    return result;
  }

  @DrawableRes
  private static int illustration(@NonNull String section)
  {
    return switch (section)
    {
      case "grove" -> R.drawable.grove_settings_anim_grove;
      case "map" -> R.drawable.grove_settings_anim_map;
      case "navigation" -> R.drawable.grove_settings_anim_navigation;
      case "privacy" -> R.drawable.grove_settings_anim_privacy;
      case "app" -> R.drawable.grove_settings_anim_app;
      default -> R.drawable.grove_settings_anim_about;
    };
  }
}
