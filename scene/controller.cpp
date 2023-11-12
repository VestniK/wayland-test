#include "controller.hpp"

namespace scene {

controller::controller() {
  using namespace std::literals;
  cube_tex_offset_update_.update({.dest = {}, .duration = 0ms});
}

void controller::operator()(gamepad::key key, bool pressed) {
  using namespace std::literals;
  if (pressed)
    return;
  switch (key) {
  case gamepad::key::A:
    cube_tex_offset_update_.update({.dest = {.0, .0}, .duration = 600ms});
    break;
  case gamepad::key::B:
    cube_tex_offset_update_.update({.dest = {.5, .0}, .duration = 600ms});
    break;
  case gamepad::key::X:
    cube_tex_offset_update_.update({.dest = {.0, .5}, .duration = 600ms});
    break;
  case gamepad::key::Y:
    cube_tex_offset_update_.update({.dest = {.5, .5}, .duration = 600ms});
    break;

  case gamepad::key::left_trg:
  case gamepad::key::right_trg:
  case gamepad::key::left_alt_trg:
  case gamepad::key::right_alt_trg:
  case gamepad::key::select:
  case gamepad::key::start:
  case gamepad::key::dpad_down:
  case gamepad::key::dpad_up:
  case gamepad::key::dpad_left:
  case gamepad::key::dpad_right:
    break;
  }
}

void controller::operator()(gamepad::axis axis, gamepad::axis2d_state state) {
  switch (axis) {
  case gamepad::axis::HAT0:
    camera_center_.update(
        glm::vec2{state.x.value * 1.5 / (state.x.maximum - state.x.minimum),
            -state.y.value * 1.5 / ((state.y.maximum - state.y.minimum))});
    break;

  case gamepad::axis::HAT2:
  case gamepad::axis::main:
  case gamepad::axis::rotational:
    break;
  }
}

void controller::operator()(gamepad::axis axis, gamepad::axis3d_state state) {
  switch (axis) {
  case gamepad::axis::main:
    cube_vel_.update(glm::vec2{-state.x.value, state.y.value} / 8000.f);
    break;

  case gamepad::axis::rotational:
    camera_center_.update(
        glm::vec2{state.x.value * 0.9 / (state.x.maximum - state.x.minimum),
            -state.y.value * 0.6 / ((state.y.maximum - state.y.minimum))});
    break;

  case gamepad::axis::HAT0:
  case gamepad::axis::HAT2:
    break;
  }
}

} // namespace scene
