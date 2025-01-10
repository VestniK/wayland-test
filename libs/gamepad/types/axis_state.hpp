#pragma once

#include <linux/input.h>

#include <array>
#include <utility>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <libs/gamepad/types/axis.hpp>

namespace gamepad {

using axis2d_state = glm::vec<2, input_absinfo>;
using axis3d_state = glm::vec<3, input_absinfo>;

class axes2d {
public:
  axis2d_state& operator[](gamepad::axis axis) noexcept {
    return axes_states_[std::to_underlying(axis) - std::to_underlying(gamepad::axis::HAT0)];
  }
  const axis2d_state& operator[](gamepad::axis axis) const noexcept {
    return axes_states_[std::to_underlying(axis) - std::to_underlying(gamepad::axis::HAT0)];
  }

  input_absinfo& operator[](gamepad::axis axis, gamepad::dimention coord) noexcept {
    return (*this)[axis][std::to_underlying(coord)];
  }

  const input_absinfo& operator[](gamepad::axis axis, gamepad::dimention coord) const noexcept {
    return (*this)[axis][std::to_underlying(coord)];
  }

private:
  std::array<axis2d_state, 2> axes_states_;
};

class axes3d {
public:
  axis3d_state& operator[](gamepad::axis axis) noexcept {
    return axes_states_[std::to_underlying(axis) - std::to_underlying(gamepad::axis::main)];
  }
  const axis3d_state& operator[](gamepad::axis axis) const noexcept {
    return axes_states_[std::to_underlying(axis) - std::to_underlying(gamepad::axis::main)];
  }

  input_absinfo& operator[](gamepad::axis axis, gamepad::dimention coord) noexcept {
    return (*this)[axis][std::to_underlying(coord)];
  }

  const input_absinfo& operator[](gamepad::axis axis, gamepad::dimention coord) const noexcept {
    return (*this)[axis][std::to_underlying(coord)];
  }

private:
  std::array<axis3d_state, 2> axes_states_;
};

} // namespace gamepad
