#include "modules/place_title_color/map/place_color.hpp"

#include "modules/logos/drape_frontend/brands.hpp"

#include "indexer/data_source.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/feature_visibility.hpp"

#include <algorithm>

namespace grove
{
namespace
{
// Labels of places have their category colour from this zoom on.
int constexpr kLabelZoom = 17;
}  // namespace

uint32_t GetPlaceColor(DataSource const & dataSource, FeatureID const & id)
{
  if (!id.IsValid())
    return 0;

  FeaturesLoaderGuard guard(dataSource, id.m_mwmId);
  auto ft = guard.GetFeatureByIndex(id.m_index);
  if (!ft)
    return 0;

  if (auto const qid = FindBrand(*ft); !qid.empty())
  {
    if (auto const color = BrandPack::Instance().GetColor(qid))
      return color;
  }

  drule::KeysT keys;
  feature::GetDrawRule(feature::TypesHolder(*ft), kLabelZoom, keys);
  // Only places with an icon have a category colour; the rest (building names, streets) keep a plain card.
  if (std::ranges::none_of(keys, [](drule::Key const & k) { return k.m_type == drule::symbol; }))
    return 0;

  auto const & rules = drule::GetCurrentRules();
  for (auto const & key : keys)
  {
    if (key.m_type != drule::caption)
      continue;
    auto const * caption = rules.Find(key)->GetCaption();
    if (caption && caption->primary)
      return caption->primary->color & 0xFFFFFF;
  }
  return 0;
}
}  // namespace grove
