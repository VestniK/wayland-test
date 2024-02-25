#version 100
precision mediump float;

uniform mat4 camera;
uniform mat4 model;
uniform sampler2D morph;

attribute vec3 position;
attribute vec3 normal;
attribute vec2 uv;
attribute vec2 idxs;

varying vec3 frag_normal;
varying vec3 frag_pos;
varying vec2 frag_uv;

void main() {
  frag_normal = normal;
  frag_pos = position;
  frag_uv = uv;

  vec3 morphed_pos = position + texture2D(morph, idxs.st).xyz;
  gl_Position = camera * model * vec4(morphed_pos, 1.);
}
