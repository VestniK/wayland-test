#version 100
precision mediump float;

@include "phong.frag"

uniform light_source light;
uniform mat4 model;
uniform mat3 norm_rotation;

varying vec3 frag_normal;
varying vec3 frag_pos;
varying vec2 frag_uv;

void main() {
  float net_dist = min(mod(frag_uv.x, 0.5), mod(frag_uv.y, 0.5));
  float grayfactor = clamp(step(0.02, net_dist), 0.6, 1.0);
  float bluefactor = clamp(step(0.02, net_dist), 0.9, 1.0);
  vec3 color = vec3(grayfactor, grayfactor, bluefactor);
  gl_FragColor = vec4(
    phong_reflect(
      light,
      color,
      vec3(model*vec4(frag_pos, 1.0)),
      normalize(norm_rotation*frag_normal)
    ),
    1.0
  );
}
