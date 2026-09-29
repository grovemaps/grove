package app.organicmaps.settings;

import android.content.Context;
import android.graphics.Typeface;
import android.util.TypedValue;
import android.view.Gravity;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.widget.TextViewCompat;
import app.organicmaps.R;
import app.organicmaps.sdk.GroveFeatures;
import app.organicmaps.util.Utils;
import com.google.android.material.bottomsheet.BottomSheetDialog;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.lang.reflect.Method;
import java.nio.charset.StandardCharsets;

// Grove[core]: a module's ⓘ: its symbol, what it does, when a switch takes effect, and its code: the hooks in upstream
// files and its own files (data/grove_modules.txt, from modules/core/tools/generate_index.py), each opening on GitHub.
// A module with settings of its own has a class app.organicmaps.settings.Grove<Module>Settings with a static
// open(Context), shown as ⚙.
final class GroveModuleSheet
{
  private static final String SOURCE = "https://github.com/grovemaps/grove/blob/grove/";

  private GroveModuleSheet() {}

  static void show(@NonNull Context context, @NonNull String module)
  {
    final String[] texts = GroveFeatures.nativeTexts(module);
    final BottomSheetDialog dialog = new BottomSheetDialog(context);
    final int pad = dp(context, 20);
    final LinearLayout list = new LinearLayout(context);
    list.setOrientation(LinearLayout.VERTICAL);
    list.setPadding(pad, pad, pad, pad);

    final TextView title =
        text(context, texts[2] + "  " + GroveModulesFragment.title(module), R.style.MwmTextAppearance_Title);
    list.addView(title);
    list.addView(text(context, texts[3], R.style.MwmTextAppearance_Body2), margin(context, 12));
    final String when = GroveFeatures.nativeNeedsRestart(module) ? "↻  " + context.getString(R.string.restart) : "⚡";
    list.addView(text(context, texts[1] + " " + texts[0] + "   " + when, R.style.MwmTextAppearance_Body3),
                 margin(context, 12));

    final Method settings = settings(module);
    if (settings != null)
    {
      final Button button = new Button(context);
      button.setText("⚙");
      button.setOnClickListener(v -> {
        try
        {
          settings.invoke(null, context);
        }
        catch (ReflectiveOperationException e)
        {
          throw new IllegalStateException(e);
        }
      });
      list.addView(button, margin(context, 12));
    }

    final String[] files = files(context, module);
    if (files != null)
    {
      list.addView(text(context, "</>", R.style.MwmTextAppearance_Body1), margin(context, 20));
      for (String path : files[0].split(";"))
        addFile(list, "✎  " + path, path);
      for (String path : files[1].split(";"))
        addFile(list, "▫  modules/" + module + "/" + path, "modules/" + module + "/" + path);
    }

    final ScrollView scroll = new ScrollView(context);
    scroll.addView(list);
    dialog.setContentView(scroll);
    dialog.show();
  }

  private static void addFile(@NonNull LinearLayout list, @NonNull String label, @NonNull String path)
  {
    if (path.isEmpty() || path.endsWith("/"))
      return;
    final Context context = list.getContext();
    final TextView view = text(context, label, R.style.MwmTextAppearance_Body3);
    view.setTypeface(Typeface.MONOSPACE);
    view.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
    view.setPadding(0, dp(context, 6), 0, dp(context, 6));
    view.setOnClickListener(v -> Utils.openUrl(context, SOURCE + path));
    list.addView(view);
  }

  // {hook files, own files}, ';'-separated, or null for a module the index doesn't have.
  @Nullable
  private static String[] files(@NonNull Context context, @NonNull String module)
  {
    try (BufferedReader reader = new BufferedReader(
             new InputStreamReader(context.getAssets().open("grove_modules.txt"), StandardCharsets.UTF_8)))
    {
      String line;
      while ((line = reader.readLine()) != null)
      {
        final String[] columns = line.split("\t", -1);
        if (columns.length == 3 && columns[0].equals(module))
          return new String[] {columns[1], columns[2]};
      }
    }
    catch (IOException ignored)
    {
      // The index is packed with the app; without it the sheet shows no files.
    }
    return null;
  }

  @Nullable
  private static Method settings(@NonNull String module)
  {
    final StringBuilder name = new StringBuilder("app.organicmaps.settings.Grove");
    for (String word : module.split("_"))
      name.append(Character.toUpperCase(word.charAt(0))).append(word.substring(1));
    try
    {
      return Class.forName(name.append("Settings").toString()).getMethod("open", Context.class);
    }
    catch (ReflectiveOperationException e)
    {
      return null;
    }
  }

  @NonNull
  private static TextView text(@NonNull Context context, @NonNull String text, int appearance)
  {
    final TextView view = new TextView(context);
    TextViewCompat.setTextAppearance(view, appearance);
    view.setText(text);
    view.setGravity(Gravity.START);
    return view;
  }

  @NonNull
  private static LinearLayout.LayoutParams margin(@NonNull Context context, int topDp)
  {
    final LinearLayout.LayoutParams params =
        new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
    params.topMargin = dp(context, topDp);
    return params;
  }

  private static int dp(@NonNull Context context, int dp)
  {
    return Math.round(dp * context.getResources().getDisplayMetrics().density);
  }
}
