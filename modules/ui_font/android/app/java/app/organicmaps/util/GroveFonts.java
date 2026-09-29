package app.organicmaps.util;

import android.app.Activity;
import android.content.Context;
import android.graphics.Typeface;
import android.os.Build;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.style.TypefaceSpan;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.res.ResourcesCompat;
import app.organicmaps.R;
import app.organicmaps.sdk.GroveFeatures;
import java.util.HashMap;
import java.util.Map;

// Grove: the app font (@font/ui: Geist, or Inter for scripts Geist lacks, see modules/ui_font/tools/android_fonts.py)
// in place of the system's sans-serif. Upstream's layouts and styles stay as they are: GroveViewInflater swaps each
// text view's typeface as it is inflated, the regular weights for @font/ui and the medium one for @font/ui_medium.
public final class GroveFonts
{
  private GroveFonts() {}

  @Nullable
  private static Map<Typeface, Typeface> sFonts;

  // Before the activity inflates anything.
  public static void install(@NonNull Activity activity)
  {
    if (GroveFeatures.isOn("ui_font"))
      activity.getTheme().applyStyle(R.style.Grove_UiFont, true);
  }

  // The app font for a view in the system's sans-serif; other fonts (emoji, monospace) stay.
  public static void apply(@NonNull View view)
  {
    if (view instanceof TextView text)
    {
      final Typeface font = fonts(view.getContext()).get(text.getTypeface());
      if (font != null)
        text.setTypeface(font);
    }
  }

  // A view group whose text views come later, like a toolbar's title, gets them in the app font too.
  static void applyToChildren(@NonNull ViewGroup group)
  {
    group.setOnHierarchyChangeListener(new ViewGroup.OnHierarchyChangeListener() {
      @Override
      public void onChildViewAdded(View parent, View child)
      {
        apply(child);
      }

      @Override
      public void onChildViewRemoved(View parent, View child)
      {}
    });
  }

  // Upstream's medium spans (sans-serif-medium) in the app font's medium weight. Spans take a typeface from
  // Android 9; older versions keep the system font.
  @NonNull
  public static SpannableStringBuilder apply(@NonNull Context context, @NonNull SpannableStringBuilder text)
  {
    if (!GroveFeatures.isOn("ui_font") || Build.VERSION.SDK_INT < Build.VERSION_CODES.P)
      return text;
    final Typeface medium = ResourcesCompat.getFont(context, R.font.ui_medium);
    if (medium == null)
      return text;
    for (TypefaceSpan span : text.getSpans(0, text.length(), TypefaceSpan.class))
    {
      if (!"sans-serif-medium".equals(span.getFamily()))
        continue;
      final int start = text.getSpanStart(span);
      final int end = text.getSpanEnd(span);
      text.removeSpan(span);
      text.setSpan(new TypefaceSpan(medium), start, end, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
    }
    return text;
  }

  // The system's sans-serif typefaces, as text views get them from a font family name or a style, and the app
  // font's for each. Typeface.create() caches them, so a view's typeface is one of these.
  @NonNull
  private static Map<Typeface, Typeface> fonts(@NonNull Context context)
  {
    if (sFonts != null)
      return sFonts;
    final Map<Typeface, Typeface> fonts = new HashMap<>();
    final Typeface ui = ResourcesCompat.getFont(context, R.font.ui);
    final Typeface uiMedium = ResourcesCompat.getFont(context, R.font.ui_medium);
    if (ui != null && uiMedium != null)
    {
      final int[] styles = {Typeface.NORMAL, Typeface.BOLD, Typeface.ITALIC, Typeface.BOLD_ITALIC};
      for (int style : styles)
      {
        final Typeface regular = Typeface.create(ui, style);
        fonts.put(Typeface.defaultFromStyle(style), regular);
        fonts.put(Typeface.create(Typeface.SANS_SERIF, style), regular);
        fonts.put(Typeface.create("sans-serif", style), regular);
        fonts.put(Typeface.create("sans-serif-light", style), regular);
        fonts.put(Typeface.create("sans-serif-medium", style), Typeface.create(uiMedium, style));
      }
      fonts.put(null, ui);
    }
    sFonts = fonts;
    return fonts;
  }
}
