#version 450
@IN(0 0 vec3 POSITION per_vertex)
@IN(0 1 vec3 NORMAL per_vertex)
@IN(0 2 vec3 TANGENT per_vertex)
@IN(0 3 vec3 BINORMAL per_vertex)
@IN(0 4 vec2 TEXCOORD_0 per_vertex)

layout(location = 0) out vec3 out_position;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_viewnormal;
layout(location = 3) out vec2 out_texcoord;

layout(set = 0, binding = 0, std140) uniform UBO {
  mat4 view;
  mat4 proj;
  vec3 L;
  vec3 power;
} uniforms;

layout(push_constant) uniform PC {
  mat4 transform;
  int albedo_id;
  int normal_id;
  int arm_id;
  float metal_factor;
  float roughness_factor;
  vec4 albedo_factor;
} push_constants;

void main() {
  vec4 wpos = push_constants.transform * vec4(POSITION, 1.0);
  out_position = wpos.xyz;
  // @TODO: Use matrix of cofactors

  out_normal = normalize((
      push_constants.transform * vec4(NORMAL, 0.0)).xyz);
  // View space normals
  out_viewnormal = normalize((
      uniforms.view * push_constants.transform * vec4(NORMAL, 0.0)).xyz);
  out_texcoord = TEXCOORD_0;

  gl_Position = uniforms.proj * uniforms.view * wpos;
}
