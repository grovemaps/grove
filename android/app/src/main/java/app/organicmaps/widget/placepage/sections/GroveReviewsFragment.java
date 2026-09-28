package app.organicmaps.widget.placepage.sections;

import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.R;
import app.organicmaps.sdk.GroveReviews;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.widget.placepage.PlacePageViewModel;
import java.text.DateFormat;
import java.util.Date;
import java.util.Locale;

// Grove: reviews of the place from Mangrove (https://mangrove.reviews), loaded when its card opens, and a link to
// write one. See libs/map/grove_reviews.hpp.
public class GroveReviewsFragment extends Fragment implements Observer<MapObject>
{
  public static final String TAG = "GROVE_REVIEWS_FRAGMENT_TAG";
  private static final int MAX_REVIEWS = 5;
  private static final int COLLAPSED_LINES = 4;

  private final Handler mMainHandler = new Handler(Looper.getMainLooper());
  private PlacePageViewModel mViewModel;
  private TextView mSummary;
  private LinearLayout mList;
  @Nullable
  private String mSubject;

  // Whether the selected place gets a reviews section.
  public static boolean isShown()
  {
    return GroveReviews.nativeGetSelectedSubject() != null;
  }

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    mViewModel = new ViewModelProvider(requireActivity()).get(PlacePageViewModel.class);
    return inflater.inflate(R.layout.grove_reviews_fragment, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mSummary = view.findViewById(R.id.grove_reviews_summary);
    mList = view.findViewById(R.id.grove_reviews_list);
    final TextView write = view.findViewById(R.id.grove_reviews_write_text);
    write.setText(getString(R.string.add_review, "Mangrove"));
    view.findViewById(R.id.grove_reviews_write).setOnClickListener(v -> {
      if (mSubject != null)
        Utils.openUrl(requireContext(), GroveReviews.nativeGetWriteReviewUrl(mSubject));
    });
  }

  @Override
  public void onStart()
  {
    super.onStart();
    mViewModel.getMapObject().observe(requireActivity(), this);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    mViewModel.getMapObject().removeObserver(this);
  }

  @Override
  public void onChanged(@Nullable MapObject mapObject)
  {
    final String subject = mapObject == null ? null : GroveReviews.nativeGetSelectedSubject();
    if (TextUtils.equals(subject, mSubject))
      return;
    mSubject = subject;
    mList.removeAllViews();
    mSummary.setText("");
    if (subject == null)
      return;

    ThreadPool.getWorker().execute(() -> {
      final GroveReviews.Review[] reviews = GroveReviews.nativeFetch(subject);
      mMainHandler.post(() -> {
        // Another place may have been selected meanwhile.
        if (isAdded() && subject.equals(mSubject))
          show(reviews);
      });
    });
  }

  private void show(@Nullable GroveReviews.Review[] reviews)
  {
    if (reviews == null || reviews.length == 0)
      return;

    int rated = 0;
    int ratingSum = 0;
    for (GroveReviews.Review r : reviews)
    {
      if (r.rating > 0)
      {
        ++rated;
        ratingSum += r.rating;
      }
    }
    // Mangrove's 0..100 ratings are 1..5 stars in steps of 25.
    final String count = "(" + reviews.length + ")";
    mSummary.setText(rated == 0 ? count
                                : String.format(Locale.getDefault(), "★ %.1f %s", 1 + ratingSum / 25f / rated, count));

    final LayoutInflater inflater = LayoutInflater.from(requireContext());
    final DateFormat dateFormat = DateFormat.getDateInstance(DateFormat.MEDIUM);
    for (int i = 0; i < Math.min(reviews.length, MAX_REVIEWS); ++i)
    {
      final GroveReviews.Review r = reviews[i];
      final View item = inflater.inflate(R.layout.grove_review_item, mList, false);
      final StringBuilder header = new StringBuilder();
      if (r.rating > 0)
        header.append("★★★★★".substring(0, r.getStars())).append("☆☆☆☆☆".substring(r.getStars())).append("  ");
      if (!r.author.isEmpty())
        header.append(r.author).append(" · ");
      header.append(dateFormat.format(new Date(r.time * 1000)));
      ((TextView) item.findViewById(R.id.grove_review_header)).setText(header);

      final TextView opinion = item.findViewById(R.id.grove_review_opinion);
      if (r.opinion.isEmpty())
        UiUtils.hide(opinion);
      else
      {
        opinion.setText(r.opinion);
        opinion.setMaxLines(COLLAPSED_LINES);
        item.setOnClickListener(
            v -> opinion.setMaxLines(opinion.getMaxLines() == COLLAPSED_LINES ? Integer.MAX_VALUE : COLLAPSED_LINES));
      }
      mList.addView(item);
    }
  }
}
