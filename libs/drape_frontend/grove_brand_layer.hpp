#pragma once

#include "drape_frontend/tile_key.hpp"

#include "indexer/feature_decl.hpp"
#include "indexer/mwm_set.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "drape/pointers.hpp"

#include <functional>
#include <set>
#include <string>
#include <vector>

namespace df
{
class EngineContext;
}  // namespace df

namespace grove
{
// The logo layer: chains' logos from zoom 12 (see grove_brands.hpp), before the map draws their places, and
// wherever they are. The places come from a list of each map's chains (map/grove_brand_places.hpp), since the map
// files index most places only from zoom 16. Places closer than a logo's width are bundled: one row of logos, one
// per chain, most common chains first. Logos are pinned (drape/grove_brand_texture.hpp).

struct BrandPlace
{
  FeatureID m_id;
  m2::PointD m_point;
  std::string m_brand;  // Wikidata id.
};

// Adds the chains' places of a map in a rect to places. Set once, before the drape engine starts.
using BrandPlacesSource = std::function<void(MwmSet::MwmId const &, m2::RectD const &, std::vector<BrandPlace> &)>;
void SetBrandPlacesSource(BrandPlacesSource source);

struct BrandCluster
{
  m2::PointD m_center;
  // One place per chain, most common chain first.
  std::vector<BrandPlace const *> m_logos;
};

// Bundles places closer than distance (mercator units) to a cluster's center, in a stable order: the same places
// always make the same clusters. A cluster shows at most maxLogos chains, and a chain's place closer than
// chainSpacing to one of its shown places is left out (0: none are).
std::vector<BrandCluster> ClusterBrandPlaces(std::vector<BrandPlace> const & places, double distance, size_t maxLogos,
                                             double chainSpacing = 0);

// Draws the logo layer of a tile read from these maps.
void DrawBrandLayer(df::TileKey const & tileKey, std::set<MwmSet::MwmId> const & mwms,
                    ref_ptr<df::EngineContext> context);
}  // namespace grove
