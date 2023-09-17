#version 100
precision mediump float;

uniform mat4 camera;
uniform mat4 model;
uniform vec2 tex_offset;

attribute vec3 position;
attribute vec3 normal;
attribute vec2 uv;

varying vec3 frag_normal;
varying vec3 frag_pos;
varying vec2 frag_uv;

void main() {
  frag_normal = normal;
  frag_pos = position;
  frag_uv = uv + tex_offset;
  gl_Position = camera * model * vec4(position.xyz, 1.);
}
