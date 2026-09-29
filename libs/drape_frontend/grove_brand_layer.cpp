#include "drape_frontend/grove_brand_layer.hpp"

#include "drape_frontend/engine_context.hpp"
#include "drape_frontend/grove_brands.hpp"
#include "drape_frontend/poi_symbol_shape.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/grove_brand_texture.hpp"
#include "drape/texture_manager.hpp"

#include "indexer/drawing_rule_def.hpp"

#include <algorithm>
#include <cmath>
#include <tuple>

namespace grove
{
namespace
{
// More chains in one spot show once the map is zoomed in further.
size_t constexpr kMaxLogos = 3;
double constexpr kLogoGapDp = 3.0;

BrandPlacesSource & Source()
{
  static BrandPlacesSource source;
  return source;
}
}  // namespace

void SetBrandPlacesSource(BrandPlacesSource source)
{
  Source() = std::move(source);
}

std::vector<BrandCluster> ClusterBrandPlaces(std::vector<BrandPlace> const & places, double distance, size_t maxLogos,
                                             double chainSpacing)
{
  // Common chains first, then by position: the clusters don't depend on the order the maps list their places in.
  auto const & pack = BrandPack::Instance();
  std::vector<BrandPlace const *> sorted;
  sorted.reserve(places.size());
  for (auto const & p : places)
    sorted.push_back(&p);
  std::ranges::sort(sorted, [&pack](BrandPlace const * a, BrandPlace const * b)
  {
    return std::tuple(pack.GetRank(a->m_brand), a->m_point.x, a->m_point.y, a->m_id) <
           std::tuple(pack.GetRank(b->m_brand), b->m_point.x, b->m_point.y, b->m_id);
  });

  struct Accumulator
  {
    m2::PointD m_seed;
    m2::PointD m_sum;
    size_t m_count = 0;
    std::vector<BrandPlace const *> m_logos;
  };
  std::vector<Accumulator> clusters;
  std::vector<BrandPlace const *> shown;
  for (auto const * place : sorted)
  {
    // Zoomed out, a chain shows once per area.
    if (chainSpacing > 0 && std::ranges::any_of(shown, [&](BrandPlace const * p)
    {
      return p->m_brand == place->m_brand && std::abs(p->m_point.x - place->m_point.x) < chainSpacing &&
             std::abs(p->m_point.y - place->m_point.y) < chainSpacing;
    }))
    {
      continue;
    }

    // Near the first place of a cluster, so a cluster doesn't creep across the map.
    auto it = std::ranges::find_if(clusters, [&](Accumulator const & c) {
      return std::abs(c.m_seed.x - place->m_point.x) < distance && std::abs(c.m_seed.y - place->m_point.y) < distance;
    });
    if (it == clusters.end())
      it = clusters.insert(clusters.end(), Accumulator{place->m_point, {}, 0, {}});

    it->m_sum += place->m_point;
    ++it->m_count;
    bool const newChain =
        std::ranges::none_of(it->m_logos, [place](BrandPlace const * p) { return p->m_brand == place->m_brand; });
    if (newChain && it->m_logos.size() < maxLogos)
    {
      it->m_logos.push_back(place);
      shown.push_back(place);
    }
  }

  std::vector<BrandCluster> result;
  result.reserve(clusters.size());
  for (auto & c : clusters)
    result.push_back({c.m_sum / static_cast<double>(c.m_count), std::move(c.m_logos)});
  return result;
}

void DrawBrandLayer(df::TileKey const & tileKey, std::set<MwmSet::MwmId> const & mwms,
                    ref_ptr<df::EngineContext> context)
{
  if (!BrandsShown() || !IsBrandLayerZoom(tileKey.m_zoomLevel) || !Source())
    return;

  m2::RectD const rect = tileKey.GetGlobalRect();
  std::vector<BrandPlace> places;
  for (auto const & mwm : mwms)
    Source()(mwm, rect, places);
  // Each place belongs to one tile.
  std::erase_if(places, [&rect](BrandPlace const & p)
  {
    return p.m_point.x < rect.minX() || p.m_point.x >= rect.maxX() || p.m_point.y < rect.minY() ||
           p.m_point.y >= rect.maxY();
  });
  if (places.empty())
    return;

  auto const & vparams = df::VisualParams::Instance();
  double const logoPx = kBrandBadgeDp * vparams.GetVisualScale();
  double const gapPx = kLogoGapDp * vparams.GetVisualScale();
  double const mercatorPerPx = rect.SizeX() / vparams.GetTileSize();
  auto const textures = context->GetTextureManager();

  double const logoWidth = logoPx * mercatorPerPx;

  df::TMapShapes shapes;
  for (auto const & cluster : ClusterBrandPlaces(places, logoWidth, kMaxLogos))
  {
    std::vector<std::pair<BrandPlace const *, std::string>> logos;
    for (auto const * place : cluster.m_logos)
    {
      std::string symbol = std::string(kBrandSymbolPrefix).append(place->m_brand);
      dp::TextureManager::SymbolRegion region;
      if (textures->GetSymbolRegionSafe(symbol, region))
        logos.emplace_back(place, std::move(symbol));
    }

    // A row of logos, centered on the places.
    double const firstX = -(logoPx + gapPx) * (static_cast<double>(logos.size()) - 1) / 2;
    for (size_t i = 0; i < logos.size(); ++i)
    {
      df::PoiSymbolViewParams params;
      // The place's overlay id: its name (from zoom 16) hangs on the logo.
      params.m_featureId = logos[i].first->m_id;
      params.m_tileCenter = rect.Center();
      params.m_depthLayer = df::DepthLayer::OverlayLayer;
      params.m_depthTestEnabled = false;
      params.m_depth = drule::kOverlaysMaxPriority - 1;
      params.m_symbolName = std::move(logos[i].second);
      params.m_offset = m2::PointF(static_cast<float>(firstX + i * (logoPx + gapPx)), 0.0f);
      auto shape = make_unique_dp<df::PoiSymbolShape>(cluster.m_center, params, tileKey, 0);
      shape->SetFeatureMinZoom(0);
      shapes.push_back(std::move(shape));
    }
  }

  for (auto const & shape : shapes)
    shape->Prepare(textures);
  if (!shapes.empty())
    context->FlushOverlays(std::move(shapes));
}
}  // namespace grove
