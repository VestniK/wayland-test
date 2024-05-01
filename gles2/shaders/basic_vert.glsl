#version 100
precision mediump float;

uniform mat4 camera;
uniform mat4 model;

attribute vec3 position;
attribute vec3 normal;

varying vec3 frag_normal;
varying vec3 frag_pos;

void main() {
  frag_normal = normal;
  frag_pos = position;
  gl_Position = camera * model * vec4(position.xyz, 1.);
}
