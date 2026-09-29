#pragma once

#include "drape_frontend/shape_view_params.hpp"

#include "drape/drape_global.hpp"
#include "modules/typography/drape/text_style.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/ftypes_matcher.hpp"

namespace grove
{
// Label typography after Apple Maps (see modules/typography/drape/text_style.hpp): semibold places with an icon,
// cities, towns and countries; spaced capitals for states, districts, neighbourhoods and streets; italic water names;
// Geist, the app's font, for road numbers, house numbers and contour heights. Everything else keeps the regular Inter
// Medium.

inline bool IsWater(feature::TypesHolder const & types)
{
  static ftypes::BaseCheckerEx const checker({{"waterway"},
                                              {"natural", "water"},
                                              {"natural", "bay"},
                                              {"natural", "strait"},
                                              {"place", "sea"},
                                              {"place", "ocean"},
                                              {"landuse", "basin"}});
  return checker(types);
}

inline uint8_t GetCaptionStyle(FeatureType & f, bool hasIcon)
{
  feature::TypesHolder const types(f);

  using ftypes::LocalityType;
  switch (ftypes::IsLocalityChecker::Instance().GetType(types))
  {
  case LocalityType::Country:
  case LocalityType::City:
  case LocalityType::Town: return kSemibold;
  case LocalityType::State: return kSpacedCaps;
  default: break;
  }

  auto const & suburbs = ftypes::IsSuburbChecker::Instance();
  for (uint32_t const t : types)
  {
    using ftypes::SuburbType;
    auto const type = suburbs.GetType(t);
    if (type == SuburbType::Suburb || type == SuburbType::Quarter || type == SuburbType::Neighbourhood)
      return kSpacedCaps;
  }

  if (hasIcon)
    return kSemibold;
  return IsWater(types) ? kItalic : kPlain;
}

inline void StyleCaption(FeatureType & f, bool hasIcon, dp::TitleDecl & title)
{
  uint8_t const style = GetCaptionStyle(f, hasIcon);
  if (style == kPlain)
    return;

  title.m_primaryText = ApplyTextStyle(style, title.m_primaryText);
  if (!title.m_secondaryText.empty())
    title.m_secondaryText = ApplyTextStyle(style, title.m_secondaryText);
}

// Road shield numbers and house numbers.
inline void StyleNumber(std::string & text)
{
  text = ApplyTextStyle(kGeist, text);
}

// Names along streets and waterways, heights along contour lines.
inline void StylePathText(FeatureType & f, df::PathTextViewParams & params)
{
  feature::TypesHolder const types(f);
  uint8_t style;
  if (ftypes::IsWayChecker::Instance()(types))
  {
    style = kSpacedCaps;
    // Capitals look bigger than mixed case at the same size: no descenders, and every letter at full height.
    params.m_textFont.m_size *= 0.9f;
  }
  else if (IsWater(types))
    style = kItalic;
  else if (ftypes::IsIsolineChecker::Instance()(types))
    style = kGeist;
  else
    return;

  // The layout shapes main and aux text as one string, which must start with the marker.
  params.m_mainText = ApplyTextStyle(style, params.ConcatRenderText());
  params.m_auxText.clear();
}
}  // namespace grove
