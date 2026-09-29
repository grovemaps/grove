package app.organicmaps.util;

import android.content.Context;
import android.graphics.Typeface;
import android.os.Build;
import android.text.style.TypefaceSpan;
import androidx.annotation.NonNull;
import androidx.core.content.res.ResourcesCompat;
import app.organicmaps.R;

// Grove: the app font for text spans, like the rest of the app (modules/ui_font/tools/android_fonts.py).
public final class GroveFonts
{
  private GroveFonts() {}

  @NonNull
  public static TypefaceSpan mediumSpan(@NonNull Context context)
  {
    // Spans take a typeface from Android 9; older versions keep the system font.
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P)
    {
      final Typeface font = ResourcesCompat.getFont(context, R.font.ui_medium);
      if (font != null)
        return new TypefaceSpan(font);
    }
    return new TypefaceSpan("sans-serif-medium");
  }
}
