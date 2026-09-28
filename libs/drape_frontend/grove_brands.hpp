#pragma once

#include "drape/grove_brand_texture.hpp"
#include "drape/texture_manager.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_meta.hpp"

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

  // The map generator drops the brand of places named like it (most of them), so the name stands in for it.
  // Matching the place type too keeps an independent shop that happens to share a chain's name plain.
  std::string_view brand = f.GetMetadata(feature::Metadata::FMD_BRAND);
  if (brand.empty())
    brand = f.GetName(StringUtf8Multilang::kDefaultCode);
  auto const & pack = BrandPack::Instance();
  if (brand.empty() || !pack.HasName(brand))
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
