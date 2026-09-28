#pragma once

#include "drape/drape_global.hpp"
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
// Places of chains show their logo badge (drape/grove_brand_texture.hpp) instead of their category icon.

// The Wikidata id of the chain a place belongs to, if its logo is in the pack, or empty.
inline std::string_view FindBrand(FeatureType & f)
{
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
    return {};

  std::vector<std::string> placeTypes;
  auto const & c = classif();
  for (uint32_t type : feature::TypesHolder(f))
  {
    ftype::TruncValue(type, 2);
    placeTypes.push_back(c.GetReadableObjectName(type));
  }

  // Map files are named "<country>_<region>", e.g. "Netherlands_North Holland_Amsterdam".
  std::string const mwm = f.GetID().GetMwmName();
  return pack.Find(brand, std::string_view(mwm).substr(0, mwm.find('_')), placeTypes);
}

// Chains show their logo from zoom 11, drawn by the logo layer (grove_brand_layer.hpp) wherever they are, whether
// or not the map's style draws their category yet. Their names show from zoom 16, under the logo.
int constexpr kBrandLayerMinZoom = 11;
int constexpr kChainNameZoom = 16;

inline bool IsBrandLayerZoom(int zoomLevel)
{
  return zoomLevel >= kBrandLayerMinZoom;
}

inline bool ShowsChainName(int zoomLevel)
{
  return zoomLevel >= kChainNameZoom;
}

// For a chain whose logo the layer draws at this zoom: true, and the logo's size in pixels. False for other places,
// and for chains whose logo doesn't fit in the texture, which keep their category icon.
inline bool ChainLogoSize(FeatureType & f, int zoomLevel, ref_ptr<dp::TextureManager> textures, m2::PointF & size)
{
  if (!IsBrandLayerZoom(zoomLevel))
    return false;
  auto const qid = FindBrand(f);
  if (qid.empty())
    return false;

  dp::TextureManager::SymbolRegion region;
  if (!textures->GetSymbolRegionSafe(std::string(kBrandSymbolPrefix).append(qid), region))
    return false;
  size = region.GetPixelSize();
  return true;
}

// A chain's name goes under its logo, and gives way to other labels and icons: the logo stays anyway.
inline void PlaceChainName(dp::TitleDecl & title, dp::Anchor below)
{
  title.m_anchor = below;
  title.m_primaryOptional = true;
  title.m_secondaryOptional = true;
}
}  // namespace grove
