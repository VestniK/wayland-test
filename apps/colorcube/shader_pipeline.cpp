#include "shader_pipeline.hpp"

#include <apps/colorcube/mesh_data.hpp>
#include <apps/colorcube/shaders.hpp>

namespace {

void apply_model_world_transformation(
    glm::mat4 transformation, gl::uniform_location<glm::mat4> model_mat_uniform,
    gl::uniform_location<glm::mat3> normal_mat_uniform
) {
  model_mat_uniform.set_value(transformation);
  normal_mat_uniform.set_value(glm::transpose(glm::inverse(glm::mat3(transformation))));
}

} // namespace

mesh::mesh(std::span<const vertex> verticies, std::span<const GLuint> indexes)
    : bufs_{gl::gen_buffers<2>()}, triangles_count_{static_cast<unsigned>(indexes.size())} {
  glBindBuffer(GL_ARRAY_BUFFER, vbo());
  glBufferData(GL_ARRAY_BUFFER, verticies.size_bytes(), verticies.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo());
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexes.size_bytes(), indexes.data(), GL_STATIC_DRAW);
}

void mesh::draw(shader_pipeline::attributes attrs) {
  glBindBuffer(GL_ARRAY_BUFFER, vbo());
  attrs.position.set_pointer(&vertex::position);
  attrs.normal.set_pointer(&vertex::normal);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo());
  glDrawElements(GL_TRIANGLES, triangles_count_, GL_UNSIGNED_INT, nullptr);
}

shader_pipeline::shader_pipeline()
    : shader_prog_{shaders::basic_vert, shaders::basic_frag},
      attributes_{
          .position = shader_prog_.get_attrib<glm::vec3>("position"),
          .normal = shader_prog_.get_attrib<glm::vec3>("normal")
      } {
  model_world_uniform_ = shader_prog_.get_uniform<glm::mat4>("model");
  norm_world_uniform_ = shader_prog_.get_uniform<glm::mat3>("norm_rotation");

  camera_uniform_ = shader_prog_.get_uniform<glm::mat4>("camera");
  color_uniform_ = shader_prog_.get_uniform<glm::vec3>("color");

  shader_prog_.use();
  shader_prog_.get_uniform<float>("light.intense").set_value(0.8);
  shader_prog_.get_uniform<float>("light.ambient").set_value(0.4);
  shader_prog_.get_uniform<float>("light.attenuation").set_value(0.01);
  shader_prog_.get_uniform<glm::vec3>("light.pos").set_value({2., 5., 15.});
};

void shader_pipeline::start_rendering(glm::mat4 camera) {
  shader_prog_.use();
  camera_uniform_.set_value(camera);
}

void shader_pipeline::draw(glm::mat4 model, glm::vec3 color, mesh& mesh) {
  apply_model_world_transformation(model, model_world_uniform_, norm_world_uniform_);
  color_uniform_.set_value(color);
  mesh.draw(attributes_);
}
