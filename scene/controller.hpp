#pragma once

#include <util/channel.hpp>

#include <gamepad/types/axis_state.hpp>
#include <gamepad/types/kyes.hpp>

namespace scene {

class controller {
public:
  controller();

  void operator()(gamepad::key key, bool pressed);
  void operator()(gamepad::axis axis, gamepad::axis2d_state state);
  void operator()(gamepad::axis axis, gamepad::axis3d_state state);

private:
};

} // namespace scene
