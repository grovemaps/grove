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
import androidx.core.content.ContextCompat;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.R;
import app.organicmaps.sdk.GroveReviews;
import app.organicmaps.sdk.GroveTripadvisor;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.widget.placepage.PlacePageViewModel;
import java.text.DateFormat;
import java.util.Date;
import java.util.Locale;

// Grove: the place's Tripadvisor rating and newest reviews, with the user's own API key (Settings), and a link to
// its Tripadvisor page as Tripadvisor's terms ask. See libs/map/grove_tripadvisor.hpp.
public class GroveTripadvisorFragment extends Fragment implements Observer<MapObject>
{
  public static final String TAG = "GROVE_TRIPADVISOR_FRAGMENT_TAG";
  private static final int COLLAPSED_LINES = 4;

  private final Handler mMainHandler = new Handler(Looper.getMainLooper());
  private PlacePageViewModel mViewModel;
  private View mRoot;
  private TextView mSummary;
  private TextView mTrend;
  private LinearLayout mList;
  @Nullable
  private String mSubject;
  @Nullable
  private String mUrl;

  // Whether the selected place gets a Tripadvisor section: with a key, for places people review.
  public static boolean isShown()
  {
    return GroveTripadvisor.nativeGetSelectedSubject() != null;
  }

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    mViewModel = new ViewModelProvider(requireActivity()).get(PlacePageViewModel.class);
    return inflater.inflate(R.layout.grove_tripadvisor_fragment, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mRoot = view;
    mSummary = view.findViewById(R.id.grove_tripadvisor_summary);
    mTrend = view.findViewById(R.id.grove_tripadvisor_trend);
    mList = view.findViewById(R.id.grove_tripadvisor_list);
    final View.OnClickListener open = v ->
    {
      if (!TextUtils.isEmpty(mUrl))
        Utils.openUrl(requireContext(), mUrl);
    };
    view.findViewById(R.id.grove_tripadvisor_header).setOnClickListener(open);
    view.findViewById(R.id.grove_tripadvisor_more).setOnClickListener(open);
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
    final String subject = mapObject == null ? null : GroveTripadvisor.nativeGetSelectedSubject();
    if (TextUtils.equals(subject, mSubject))
      return;
    mSubject = subject;
    show(null);
    if (subject == null)
      return;

    ThreadPool.getWorker().execute(() -> {
      final GroveTripadvisor.Result result = GroveTripadvisor.nativeFetch(subject);
      mMainHandler.post(() -> {
        // Another place may have been selected meanwhile.
        if (isAdded() && subject.equals(mSubject))
          show(result);
      });
    });
  }

  private void show(@Nullable GroveTripadvisor.Result result)
  {
    mList.removeAllViews();
    UiUtils.hide(mTrend);
    mUrl = result == null ? null : result.url;
    // Only places Tripadvisor knows get a section.
    UiUtils.showIf(result != null, mRoot);
    if (result == null)
      return;

    final String count = "(" + result.count + ")";
    mSummary.setText(result.stars == 0 ? count : String.format(Locale.getDefault(), "★ %.1f %s", result.stars, count));
    if (result.recentStars > 0)
    {
      mTrend.setText(getString(R.string.reviews_last_6_months,
                               String.format(Locale.getDefault(), "★ %.1f %s (%d)", result.recentStars,
                                             result.better ? "↑" : "↓", result.recentCount)));
      mTrend.setTextColor(
          ContextCompat.getColor(requireContext(), result.better ? R.color.base_green : R.color.base_red));
      UiUtils.show(mTrend);
    }

    final LayoutInflater inflater = LayoutInflater.from(requireContext());
    final DateFormat dateFormat = DateFormat.getDateInstance(DateFormat.MEDIUM);
    for (GroveReviews.Review r : result.reviews)
    {
      final View item = inflater.inflate(R.layout.grove_review_item, mList, false);
      final StringBuilder header = new StringBuilder();
      if (r.rating > 0)
        header.append("★★★★★".substring(0, r.getStars())).append("☆☆☆☆☆".substring(r.getStars())).append("  ");
      if (!r.author.isEmpty())
        header.append(r.author).append(" · ");
      if (r.time > 0)
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
