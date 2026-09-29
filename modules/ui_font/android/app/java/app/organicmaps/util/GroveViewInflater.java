package app.organicmaps.util;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.AppCompatAutoCompleteTextView;
import androidx.appcompat.widget.AppCompatButton;
import androidx.appcompat.widget.AppCompatCheckBox;
import androidx.appcompat.widget.AppCompatCheckedTextView;
import androidx.appcompat.widget.AppCompatEditText;
import androidx.appcompat.widget.AppCompatMultiAutoCompleteTextView;
import androidx.appcompat.widget.AppCompatRadioButton;
import androidx.appcompat.widget.AppCompatTextView;
import androidx.appcompat.widget.AppCompatToggleButton;
import androidx.appcompat.widget.Toolbar;
import com.google.android.material.theme.MaterialComponentsViewInflater;
import java.lang.reflect.Constructor;
import java.util.HashMap;
import java.util.Map;

// Grove: Material's view inflater, with text views in the app font (GroveFonts). The Grove.UiFont theme overlay
// names it, only with the ui_font module on.
public class GroveViewInflater extends MaterialComponentsViewInflater
{
  // Views named by their class in layouts (MaterialTextView, Chip, the app's own text views, toolbars): their
  // constructor, or null for other views, which the layout inflater creates as usual.
  private static final Map<String, Constructor<? extends View>> sConstructors = new HashMap<>();

  @NonNull
  private static <T extends View> T font(@NonNull T view)
  {
    GroveFonts.apply(view);
    return view;
  }

  @NonNull
  @Override
  protected AppCompatTextView createTextView(Context context, AttributeSet attrs)
  {
    return font(super.createTextView(context, attrs));
  }

  @NonNull
  @Override
  protected AppCompatButton createButton(@NonNull Context context, @NonNull AttributeSet attrs)
  {
    return font(super.createButton(context, attrs));
  }

  @NonNull
  @Override
  protected AppCompatEditText createEditText(Context context, AttributeSet attrs)
  {
    return font(super.createEditText(context, attrs));
  }

  @NonNull
  @Override
  protected AppCompatCheckBox createCheckBox(Context context, AttributeSet attrs)
  {
    return font(super.createCheckBox(context, attrs));
  }

  @NonNull
  @Override
  protected AppCompatRadioButton createRadioButton(Context context, AttributeSet attrs)
  {
    return font(super.createRadioButton(context, attrs));
  }

  @NonNull
  @Override
  protected AppCompatCheckedTextView createCheckedTextView(Context context, AttributeSet attrs)
  {
    return font(super.createCheckedTextView(context, attrs));
  }

  @NonNull
  @Override
  protected AppCompatAutoCompleteTextView createAutoCompleteTextView(@NonNull Context context,
                                                                     @Nullable AttributeSet attrs)
  {
    return font(super.createAutoCompleteTextView(context, attrs));
  }

  @NonNull
  @Override
  protected AppCompatMultiAutoCompleteTextView createMultiAutoCompleteTextView(Context context, AttributeSet attrs)
  {
    return font(super.createMultiAutoCompleteTextView(context, attrs));
  }

  @NonNull
  @Override
  protected AppCompatToggleButton createToggleButton(Context context, AttributeSet attrs)
  {
    return font(super.createToggleButton(context, attrs));
  }

  @Nullable
  @Override
  protected View createView(Context context, String name, AttributeSet attrs)
  {
    final View view = super.createView(context, name, attrs);
    if (view != null || name.indexOf('.') < 0)
      return view;
    final Constructor<? extends View> constructor = constructor(context, name);
    if (constructor == null)
      return null;
    try
    {
      final View created = constructor.newInstance(context, attrs);
      if (created instanceof Toolbar toolbar)
        GroveFonts.applyToChildren(toolbar);
      return font(created);
    }
    catch (ReflectiveOperationException e)
    {
      return null;
    }
  }

  @Nullable
  private static Constructor<? extends View> constructor(@NonNull Context context, @NonNull String name)
  {
    synchronized (sConstructors)
    {
      if (sConstructors.containsKey(name))
        return sConstructors.get(name);
      Constructor<? extends View> constructor = null;
      try
      {
        final Class<?> clazz = Class.forName(name, false, context.getClassLoader());
        if (TextView.class.isAssignableFrom(clazz) || Toolbar.class.isAssignableFrom(clazz))
        {
          constructor = clazz.asSubclass(View.class).getConstructor(Context.class, AttributeSet.class);
          constructor.setAccessible(true);
        }
      }
      catch (ReflectiveOperationException | LinkageError ignored)
      {
        // Not a view class we know how to make: the layout inflater does it.
      }
      sConstructors.put(name, constructor);
      return constructor;
    }
  }
}
