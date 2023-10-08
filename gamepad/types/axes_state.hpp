#pragma once

#include <array>

#include <glm/vec2.hpp>

#include <util/channel.hpp>

#include <gamepad/types/axis.hpp>

namespace gamepad {

struct axis_value_consumer {
  glm::ivec2 current{};
  value_update_channel<glm::ivec2>* channel;
};

class axes_state {
public:
  void set_axis_channel(
      gamepad::axis axis, value_update_channel<glm::ivec2>& channel) noexcept {
    get(axis).channel = &channel;
  }

  void reset_axis_channel(gamepad::axis axis) noexcept {
    get(axis).channel = nullptr;
  }

  axis_value_consumer& get(gamepad::axis axis) noexcept {
    return values_[static_cast<std::underlying_type_t<gamepad::axis>>(axis)];
  }

  const axis_value_consumer& get(gamepad::axis axis) const noexcept {
    return values_[static_cast<std::underlying_type_t<gamepad::axis>>(axis)];
  }

private:
  std::array<axis_value_consumer, 4> values_;
};

} // namespace gamepag
