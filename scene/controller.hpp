#pragma once

#include <libs/sync/channel.hpp>

#include <libs/gamepad/types/axis_state.hpp>
#include <libs/gamepad/types/kyes.hpp>

#include <scene/anime.hpp>

namespace scene {

class controller {
public:
  controller();

  void operator()(gamepad::key key, bool pressed);
  void operator()(gamepad::axis axis, gamepad::axis2d_state state);
  void operator()(gamepad::axis axis, gamepad::axis3d_state state);

  const auto get_landscape_color_update() const noexcept {
    return landscape_color_.get_update();
  }

  const auto get_cube_color_update() const noexcept {
    return cube_color_.get_update();
  }

  glm::vec2 current_cube_vel() const noexcept {
    return cube_vel_.get_current();
  }

private:
  value_update_channel<animate_to> landscape_color_;
  value_update_channel<animate_to> cube_color_;
  value_update_channel<glm::vec2> cube_vel_;
};

} // namespace scene
