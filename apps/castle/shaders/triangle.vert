#version 450

layout(binding = 0) uniform world_transformations {
    mat4 camera;
    mat4 model;
    mat3 norm_rotation;
} mvp;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec3 frag_normal;
layout(location = 1) out vec3 frag_pos;
layout(location = 2) out vec2 frag_uv;

void main() {
    frag_normal = in_normal;
    frag_pos = in_position;
    frag_uv = in_uv;
    gl_Position = mvp.camera * mvp.model * vec4(in_position.xyz, 1.);
}
