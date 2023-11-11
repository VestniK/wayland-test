#pragma once

namespace gamepad {

enum class axis {
  main,
  rotational,
  HAT0,
  HAT2,
};

constexpr int axis_dimrntions(axis a) noexcept {
  return a < axis::HAT0 ? 3 : 2;
}

enum class dimention { x, y, z };

} // namespace gamepad
