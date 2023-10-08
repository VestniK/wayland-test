#include "controller.hpp"

namespace scene {

controller::controller() {
  using namespace std::literals;
  axes_.set_axis_channel(gamepad::axis::main, cube_vel_);
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

} // namespace scene
