#include "scene.hpp"

namespace scene {

void update_world(frames_clock::time_point ts, world_transformations& world) noexcept {
  using namespace std::literals;
  const float phase = static_cast<float>(
      2 * M_PI * (ts.time_since_epoch() % 5s).count() / float(frames_clock::duration{5s}.count())
  );
  const auto model = glm::translate(glm::mat4{1.}, glm::vec3{std::cos(phase), 0., 0.});
  world.model = model;
  world.norm_rotation = glm::transpose(glm::inverse(glm::mat3(model)));
}

std::array<glm::mat4, 4> catapult::sprites_transformations() const noexcept {
  return std::array<glm::mat4, 4>{
      front_wheel_transformation(),
      rear_wheel_transformation(),
      platform_transformation(),
      arm_transformation(),
  };
}

glm::mat4 catapult::front_wheel_transformation() const noexcept {
  return glm::translate(
      glm::rotate(glm::translate(glm::mat4{1.}, {wheel_radius, wheel_radius, 0}), wheel_angle, {0, 0, 1}),
      glm::vec3{anchor_point.x, anchor_point.y, 0.}
  );
}

glm::mat4 catapult::rear_wheel_transformation() const noexcept {
  return glm::translate(
      glm::rotate(glm::translate(glm::mat4{1.}, {wheel_radius, wheel_radius, 0}), wheel_angle, {0, 0, 1}),
      glm::vec3{anchor_point.x - 3.2, anchor_point.y, 0.}
  );
}

glm::mat4 catapult::platform_transformation() const noexcept {
  return glm::translate(
      glm::scale(glm::mat4{1.}, {1. / 2.5, 1, 1}), {anchor_point.x - .35, anchor_point.y + .5, 0}
  );
}

glm::mat4 catapult::arm_transformation() const noexcept {
  return glm::translate(
      glm::rotate(
          glm::translate(glm::scale(glm::mat4{1.}, {1. / 2.9, 1 / 1.2, 1}), {0.5, 0.5, 0}),
          static_cast<float>(M_PI / 4. + (M_PI / 10.) * arm_position), {0, 0, 1}
      ),
      {anchor_point.x - 1.3, anchor_point.y + 0.4, 0}
  );
}

glm::mat4 setup_camera(vk::Extent2D sz) noexcept {
  constexpr auto camera_pos = glm::vec3{0., 0., 40.};
  constexpr auto camera_up_direction = glm::vec3{0., 1., 0.};
  constexpr auto camera_look_at = glm::vec3{10., 10., 0.};
  return glm::perspectiveFov<float>(M_PI / 6., sz.width, sz.height, 20.f, 60.f) *
         glm::lookAt(camera_pos, camera_look_at, camera_up_direction);
}

vlk::mesh_data<vertex, uint16_t> make_paper() {
  vlk::mesh_data<vertex, uint16_t> res;

  constexpr int x_segments = 170;
  constexpr int y_segments = 210;
  auto pos2idx = [](int x, int y) -> uint16_t { return y * x_segments + x; };

  for (int y = 0; y < y_segments; ++y) {
    for (int x = 0; x < x_segments; ++x) {
      res.vertices.push_back(
          {.position = {0.1 * x, 0.1 * y, 0.}, .normal = {0., 0., 1.}, .uv = {0.1 * x, 0.1 * y}}
      );
      if (y > 0 && x < x_segments - 1) {
        res.indices.push_back(pos2idx(x, y - 1));
        res.indices.push_back(pos2idx(x + 1, y - 1));
        res.indices.push_back(pos2idx(x, y));

        res.indices.push_back(pos2idx(x + 1, y - 1));
        res.indices.push_back(pos2idx(x + 1, y));
        res.indices.push_back(pos2idx(x, y));
      }
    }
  }
  return res;
}

} // namespace scene
