layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_colorTexCoords;
layout (location = 0) out vec2 v_colorTexCoords;
layout (location = 1) out float v_intensity;

layout (binding = 0) uniform UBO
{
  mat4 u_modelView;
  mat4 u_projection;
  mat4 u_pivotTransform;
  vec2 u_contrastGamma;
  float u_opacity;
  float u_zScale;
  float u_interpolation;
  float u_isOutlinePass;
};

// Grove: sun from the upper left of the screen, as in shaded relief maps.
const vec4 kNormalizedLightDir = vec4(0.4472, -0.4472, 0.7746, 0.0);
// Grove: walls darken toward the ground, so buildings read as standing on it.
const float kGroundShade = 0.8;

void main()
{
  vec4 pos = vec4(a_position, 1.0) * u_modelView;
  vec4 normal = vec4(a_position + a_normal, 1.0) * u_modelView;
  normal.xyw = (normal * u_projection).xyw;
  normal.z = normal.z * u_zScale;
  pos.xyw = (pos * u_projection).xyw;
  pos.z = a_position.z * u_zScale;
  vec4 normDir = normal - pos;
  if (dot(normDir, normDir) != 0.0)
    v_intensity = max(0.0, -dot(kNormalizedLightDir, normalize(normDir)));
  else
    v_intensity = 0.0;
  // Grove: brightness from ambient and sun light; roofs ~1.0, sunlit walls ~0.95, walls in shade 0.72.
  v_intensity = 0.72 + 0.36 * v_intensity;
  if (a_position.z == 0.0)
    v_intensity *= kGroundShade;
  gl_Position = u_pivotTransform * pos;
#ifdef VULKAN
  gl_Position.y = -gl_Position.y;
  gl_Position.z = (gl_Position.z  + gl_Position.w) * 0.5;
#endif
  v_colorTexCoords = a_colorTexCoords;
}
