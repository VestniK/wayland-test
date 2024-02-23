#include "controller.hpp"

namespace scene {

controller::controller() {}

void controller::operator()(gamepad::key key, bool pressed) {
  if (pressed)
    return;
  switch (key) {
  case gamepad::key::A:
  case gamepad::key::B:
  case gamepad::key::X:
  case gamepad::key::Y:
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
  case gamepad::axis::HAT2:
  case gamepad::axis::main:
  case gamepad::axis::rotational:
    break;
  }
}

void controller::operator()(gamepad::axis axis, gamepad::axis3d_state state) {
  switch (axis) {
  case gamepad::axis::main:
  case gamepad::axis::rotational:
  case gamepad::axis::HAT0:
  case gamepad::axis::HAT2:
    break;
  }
}

} // namespace scene
