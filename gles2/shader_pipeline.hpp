#pragma once

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <gles2/resource.hpp>
#include <gles2/textures.hpp>

struct vertex;
class mesh;

class shader_pipeline {
public:
  struct attributes {
    gl::attrib_location<glm::vec3> position;
    gl::attrib_location<glm::vec3> normal;
    gl::attrib_location<glm::vec2> uv;
    gl::attrib_location<glm::vec2> idxs;
  };

  shader_pipeline();

  void start_rendering(glm::mat4 camera);
  void draw(glm::mat4 model, mesh& mesh);

  void bind_morph(gl::texture_sampler tex);

private:
  gl::shader_program shader_prog_;
  gl::uniform_location<glm::mat4> camera_uniform_;
  gl::uniform_location<glm::mat4> model_world_uniform_;
  gl::uniform_location<glm::mat3> norm_world_uniform_;
  gl::uniform_location<gl::texture_sampler> morph_uniform_;
  attributes attributes_;
};

class mesh {
public:
  constexpr mesh() noexcept = default;
  explicit mesh(
      std::span<const vertex> verticies, std::span<const GLuint> indexes);

  void draw(shader_pipeline::attributes attrs);

private:
  GLuint vbo() const noexcept { return bufs_[0].native_handle(); }
  GLuint ibo() const noexcept { return bufs_[1].native_handle(); }

private:
  gl::buffers<2> bufs_;
  unsigned triangles_count_ = 0;
};
