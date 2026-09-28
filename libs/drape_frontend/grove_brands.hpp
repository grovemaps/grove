#pragma once

#include "drape/grove_brand_texture.hpp"
#include "drape/texture_manager.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"

#include "coding/string_utf8_multilang.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace grove
{
// Places of chains show their logo badge instead of the category icon when zoomed in (see
// drape/grove_brand_texture.hpp). Zoomed out they keep category icons and dots.
int constexpr kMinBrandZoom = 16;

// Replaces symbolName with the brand badge of the feature, if it has one and there is room in the badge texture.
inline void UseBrandBadge(FeatureType & f, int zoomLevel, ref_ptr<dp::TextureManager> textures,
                          std::string & symbolName)
{
  if (zoomLevel < kMinBrandZoom)
    return;

  // Brands are matched by name: the map generator drops the brand tag of places named like it (most of them),
  // and reading the stored brand of every place would slow tile loading. A branch name like "Albert Heijn Dam"
  // matches by its leading words. Matching the place type too keeps an independent shop that happens to share
  // a chain's name plain.
  std::string_view brand = f.GetName(StringUtf8Multilang::kDefaultCode);
  auto const & pack = BrandPack::Instance();
  while (!brand.empty() && !pack.HasName(brand))
  {
    auto const space = brand.rfind(' ');
    brand = space == std::string_view::npos ? std::string_view{} : brand.substr(0, space);
  }
  if (brand.empty())
    return;

  std::vector<std::string> placeTypes;
  auto const & c = classif();
  for (uint32_t type : feature::TypesHolder(f))
  {
    ftype::TruncValue(type, 2);
    placeTypes.push_back(c.GetReadableObjectName(type));
  }

  // Map files are named "<country>_<region>", e.g. "Netherlands_North Holland_Amsterdam".
  std::string const mwm = f.GetID().GetMwmName();
  auto const qid = pack.Find(brand, std::string_view(mwm).substr(0, mwm.find('_')), placeTypes);
  if (qid.empty())
    return;

  std::string badge = std::string(kBrandSymbolPrefix).append(qid);
  dp::TextureManager::SymbolRegion region;
  if (textures->GetSymbolRegionSafe(badge, region))
    symbolName = std::move(badge);
}
}  // namespace grove
