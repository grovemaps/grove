#pragma once

#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "drape/glsl_types.hpp"
#include "drape/pointers.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"  // Grove[see_through_buildings]
#include "geometry/triangle2d.hpp"

#include <vector>

namespace df
{

struct BuildingOutline
{
  buffer_vector<m2::PointD, kBuildingOutlineSize> m_vertices;
  std::vector<int> m_indices;
  std::vector<m2::PointD> m_normals;
  bool m_generateOutline = false;
};

// Returns the world-space anchor for one axis of a hatching-area's repeated mask UVs.
//
// Hatching UVs are 'maxU * (vertex - anchor)' with maxU = baseGtoPScale / maskSizePx, so one mask repeat
// spans 'maskSizePx / baseGtoPScale' world units. Anchoring to the clipped per-shape bbox.min is unstable:
// non-building area triangles are clipped to the tile rect (ApplyAreaFeature::operator(), via
// m2::ClipTriangleByRect), so bbox.min jumps whenever a feature is re-tiled or the LOD changes, snapping
// the dash phase (see issue #12804).
//
// This snaps the anchor down to the nearest multiple of the mask period, i.e. a global, tile-independent
// grid. The sampled texel (UV mod 1) then depends only on the world coordinate, so the pattern stays
// continuous across tile seams, while '(vertex - anchor)' stays small (< bbox size + one period) to keep
// float precision in the GPU attribute.
double CalcHatchingPhaseAnchor(double bboxMin, uint32_t maskSizePx, double baseGtoPScale);

class AreaShape : public MapShape
{
public:
  AreaShape(std::vector<m2::PointD> triangleList, BuildingOutline && buildingOutline, AreaViewParams const & params);

  // Grove: see modules/see_through_buildings/drape_frontend/buildings.hpp.
  bool IsBuilding3D() const { return m_params.m_is3D; }
  // Grove: the footprint's bounding box, see modules/see_through_buildings/drape_frontend/buildings.hpp.
  m2::RectD GetBounds() const
  {
    m2::RectD r;
    for (auto const & v : m_vertexes)
      r.Add(v);
    return r;
  }
  bool Contains(m2::PointD const & pt) const
  {
    for (size_t i = 0; i + 2 < m_vertexes.size(); i += 3)
      if (m2::IsPointInsideTriangle(pt, m_vertexes[i], m_vertexes[i + 1], m_vertexes[i + 2]))
        return true;
    return false;
  }
  void SetAlpha(uint8_t alpha)
  {
    auto const & c = m_params.m_color;
    m_params.m_color = dp::Color(c.GetRed(), c.GetGreen(), c.GetBlue(), alpha);
  }

  void Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
            ref_ptr<dp::TextureManager> textures) const override;

private:
  glsl::vec2 ToShapeVertex2(m2::PointD const & vertex) const
  {
    return glsl::ToVec2(ConvertToLocal(vertex, m_params.m_tileCenter, kShapeCoordScalar));
  }
  glsl::vec3 ToShapeVertex3(m2::PointD const & vertex) const { return {ToShapeVertex2(vertex), m_params.m_depth}; }

  void DrawArea(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher, m2::PointD const & colorUv,
                m2::PointD const & outlineUv, ref_ptr<dp::Texture> texture) const;
  void DrawMwmBorderArea(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher, m2::PointD const & colorUv,
                         ref_ptr<dp::Texture> texture) const;
  void DrawArea3D(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher, m2::PointD const & colorUv,
                  m2::PointD const & outlineUv, ref_ptr<dp::Texture> texture) const;
  void DrawPatternArea(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher, m2::PointD const & colorUv,
                       ref_ptr<dp::Texture> texture, std::string_view patternKey) const;

  std::vector<m2::PointD> m_vertexes;
  BuildingOutline m_buildingOutline;
  AreaViewParams m_params;
};
}  // namespace df
