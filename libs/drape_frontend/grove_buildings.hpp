#pragma once

#include "drape_frontend/area_shape.hpp"
#include "drape_frontend/engine_context.hpp"
#include "drape_frontend/map_shape.hpp"

#include "geometry/point2d.hpp"

#include <algorithm>
#include <vector>

namespace grove
{
// 3D buildings with a place inside (a shop, café, museum...) are see-through, so its dot and the ground stay
// visible, as in Apple Maps. RuleDrawer reads a tile's features in id order, so a building can come before the
// places inside it: the tile's 3D buildings are held back and flushed when the tile ends.
class HollowBuildings
{
public:
  // Holds a 3D building shape; false for other shapes.
  bool Hold(drape_ptr<df::MapShape> & shape)
  {
    auto * area = dynamic_cast<df::AreaShape *>(shape.get());
    if (area == nullptr || !area->IsBuilding3D())
      return false;
    m_buildings.push_back(std::move(shape));
    return true;
  }

  // A place of the tile: a point with an icon or a name.
  void AddPlace(m2::PointD const & point) { m_places.push_back(point); }

  void Flush(ref_ptr<df::EngineContext> context)
  {
    if (m_buildings.empty())
      return;
    // Places sorted from west to east: each building only tests the places within its bounding box.
    std::ranges::sort(m_places, {}, &m2::PointD::x);
    for (auto & shape : m_buildings)
    {
      auto * area = static_cast<df::AreaShape *>(shape.get());
      m2::RectD const bounds = area->GetBounds();
      for (auto p = std::ranges::lower_bound(m_places, bounds.minX(), {}, &m2::PointD::x);
           p != m_places.end() && p->x <= bounds.maxX(); ++p)
      {
        if (p->y >= bounds.minY() && p->y <= bounds.maxY() && area->Contains(*p))
        {
          area->SetAlpha(kHollowAlpha);
          break;
        }
      }
      shape->Prepare(context->GetTextureManager());
    }
    context->Flush(std::move(m_buildings));
    m_buildings.clear();
  }

private:
  // Of 255; the 3D layer's own opacity applies on top.
  static uint8_t constexpr kHollowAlpha = 90;

  df::TMapShapes m_buildings;
  std::vector<m2::PointD> m_places;
};
}  // namespace grove
