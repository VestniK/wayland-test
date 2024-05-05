#include "controller.hpp"

using namespace std::literals;

namespace scene {

controller::controller() {
  cube_color_.update({.dest = {.9, 0.7, 0.7}, .duration = 0ms});
  landscape_color_.update({.dest = {1., 1., 0.4}, .duration = 0ms});
}

void controller::operator()(gamepad::key key, bool pressed) {
  if (pressed)
    return;
  switch (key) {
  case gamepad::key::A:
    cube_color_.update({.dest = {.0, 0.9, 0.0}, .duration = 600ms});
    break;
  case gamepad::key::B:
    cube_color_.update({.dest = {.9, 0.0, 0.0}, .duration = 600ms});
    break;
  case gamepad::key::X:
    cube_color_.update({.dest = {.0, .0, .9}, .duration = 600ms});
    break;
  case gamepad::key::Y:
    cube_color_.update({.dest = {0.8, .8, 0.4}, .duration = 600ms});
    break;
  case gamepad::key::left_trg:
    landscape_color_.update({.dest = {.8, .0, .0}, .duration = 600ms});
    break;
  case gamepad::key::right_trg:
    landscape_color_.update({.dest = {.0, .8, .0}, .duration = 600ms});
    break;
  case gamepad::key::left_alt_trg:
    landscape_color_.update({.dest = {.4, .0, .0}, .duration = 600ms});
    break;
  case gamepad::key::right_alt_trg:
    landscape_color_.update({.dest = {.0, .4, .0}, .duration = 600ms});
    break;
  case gamepad::key::select:
    cube_color_.update({.dest = {.2, .2, .2}, .duration = 400ms});
    break;
  case gamepad::key::start:
    cube_color_.update({.dest = {.9, 0.7, 0.7}, .duration = 500ms});
    landscape_color_.update({.dest = {1., 1., 0.4}, .duration = 500ms});
    break;
  case gamepad::key::dpad_down:
  case gamepad::key::dpad_up:
  case gamepad::key::dpad_left:
  case gamepad::key::dpad_right:
    break;
  }
}

void controller::operator()(gamepad::axis axis, gamepad::axis2d_state state) {
  switch (axis) {
  case gamepad::axis::main:
    cube_vel_.update(glm::vec2{-state.x.value, state.y.value} / 8000.f);
    break;
  case gamepad::axis::HAT0:
  case gamepad::axis::HAT2:
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
  case gamepad::axis::HAT0:
  case gamepad::axis::HAT2:
    break;
  }
}

} // namespace scene
