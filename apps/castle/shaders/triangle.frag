#version 450

layout(binding = 0) uniform world_transformations {
    mat4 camera;
    mat4 model;
    mat4 norm_rotation;
} mvp;

layout(binding = 1) uniform light_source {
  vec3 pos;
  float intense;
  float ambient;
  float attenuation;
} light;

layout(binding = 2) uniform sampler sprite_smp;

#define SPRITES_COUNT 5
layout(binding = 3) uniform sprites_transform {
  mat4 models[SPRITES_COUNT];
} sprites_tr;

layout(binding = 4) uniform texture2D sprites[SPRITES_COUNT];

layout(location = 0) in vec3 frag_normal;
layout(location = 1) in vec3 frag_pos;
layout(location = 2) in vec2 frag_uv;

layout(location = 0) out vec4 out_color;

vec3 phong_reflect(vec3 surf_color, vec3 world_pos, vec3 world_normal) {
  vec3 light_direction = normalize(light.pos - world_pos);
  float brightnes = light.intense*dot(world_normal, light_direction);
  brightnes = clamp(brightnes, 0.0, 1.0);
  vec3 diffuse = brightnes*surf_color;
  vec3 ambient = light.intense*light.ambient*surf_color;
  float attenuation = 1.0/(1.0 + light.attenuation*pow(length(light.pos - world_pos), 2.0));
  return ambient + attenuation*diffuse;
}

void main() {
  float net_dist = min(abs(fract(2*frag_uv.x) - 0.5), abs(fract(2*frag_uv.y) - 0.5));
  float grayfactor = clamp(smoothstep(0., 0.05, net_dist), 0.6, 1.0);
  float bluefactor = clamp(smoothstep(0., 0.05, net_dist), 0.9, 1.0);
  vec3 color = vec3(grayfactor, grayfactor, bluefactor);

  for (int i = 0; i < SPRITES_COUNT; i++) {
    vec4 sprite_color = texture(
      sampler2D(sprites[i], sprite_smp),
      (sprites_tr.models[i]*vec4(frag_uv, 0, 1)).xy
    );
    color = mix(color, sprite_color.rgb, sprite_color.a);
  }

  out_color = vec4(
    phong_reflect(
      color,
      vec3(mvp.model * vec4(frag_pos, 1.0)),
      normalize(mat3(mvp.norm_rotation) * frag_normal)
    ),
    1.0
  );
}
