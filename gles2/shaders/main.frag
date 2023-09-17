#version 100
precision mediump float;

@include(phong.frag)

uniform light_source light;
uniform mat4 model;
uniform mat3 norm_rotation;

varying vec3 frag_normal;
varying vec3 frag_pos;
varying vec2 frag_uv;

uniform sampler2D texture_data;

void main() {
  vec3 color = texture2D(texture_data, frag_uv).rgb;
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
