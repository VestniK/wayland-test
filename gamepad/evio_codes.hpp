#pragma once

#include <linux/input-event-codes.h>

#include <cstdint>
#include <expected>
#include <optional>
#include <string_view>

#include <gamepad/types/axis.hpp>
#include <gamepad/types/kyes.hpp>

namespace evio {

using std::literals::operator""sv;

enum class error {
  no_z_for_2d_axis,
  unknown_key_code,
};

struct axe {
  uint16_t code;
  std::string_view define_name;
};

constexpr std::string_view ev_type_name(std::uint16_t type) noexcept {
  switch (type) {
  case EV_SYN:
    return "EV_SYN"sv;
  case EV_KEY:
    return "EV_KEY"sv;
  case EV_REL:
    return "EV_REL"sv;
  case EV_ABS:
    return "EV_ABS"sv;
  case EV_MSC:
    return "EV_MSC"sv;
  case EV_SW:
    return "EV_SW"sv;
  case EV_LED:
    return "EV_LED"sv;
  case EV_SND:
    return "EV_SND"sv;
  case EV_REP:
    return "EV_REP"sv;
  case EV_FF:
    return "EV_FF"sv;
  case EV_PWR:
    return "EV_PWR"sv;
  case EV_FF_STATUS:
    return "EV_FF_STATUS"sv;
  }
  return "unknown evdev event type"sv;
}

struct axis_dim {
  gamepad::axis axis;
  gamepad::dimention dim;
};

constexpr auto evcode2axis(uint16_t evcode) noexcept {
  using enum gamepad::axis;
  using enum gamepad::dimention;
  std::optional<axis_dim> res;
  switch (evcode) {
  case ABS_X:
    res = {.axis = main, .dim = x};
    break;
  case ABS_Y:
    res = {.axis = main, .dim = y};
    break;
  case ABS_Z:
    res = {.axis = main, .dim = z};
    break;
  case ABS_RX:
    res = {.axis = rotational, .dim = x};
    break;
  case ABS_RY:
    res = {.axis = rotational, .dim = y};
    break;
  case ABS_RZ:
    res = {.axis = rotational, .dim = z};
    break;
  case ABS_HAT0X:
    res = {.axis = HAT0, .dim = x};
    break;
  case ABS_HAT0Y:
    res = {.axis = HAT0, .dim = y};
    break;
  case ABS_HAT2X:
    res = {.axis = HAT2, .dim = x};
    break;
  case ABS_HAT2Y:
    res = {.axis = HAT2, .dim = y};
    break;
  }
  return res;
}

constexpr std::expected<evio::axe, evio::error> axis2evcode(
    gamepad::axis axis, gamepad::dimention coord) noexcept {
  using enum gamepad::dimention;
  evio::axe res;
  switch (axis) {
  case gamepad::axis::main:
    switch (coord) {
    case x:
      res = {.code = ABS_X, .define_name = "ABS_X"sv};
      break;
    case y:
      res = {.code = ABS_Y, .define_name = "ABS_Y"sv};
      break;
    case z:
      res = {.code = ABS_Z, .define_name = "ABS_Z"sv};
      break;
    }
    break;
  case gamepad::axis::rotational:
    switch (coord) {
    case x:
      res = {.code = ABS_RX, .define_name = "ABS_RX"sv};
      break;
    case y:
      res = {.code = ABS_RY, .define_name = "ABS_RY"sv};
      break;
    case z:
      res = {.code = ABS_RZ, .define_name = "ABS_RZ"sv};
      break;
    }
    break;
  case gamepad::axis::HAT0:
    switch (coord) {
    case x:
      res = {.code = ABS_HAT0X, .define_name = "ABS_HAT0X"sv};
      break;
    case y:
      res = {.code = ABS_HAT0Y, .define_name = "ABS_HAT0Y"sv};
      break;
    case z:
      return std::unexpected(error::no_z_for_2d_axis);
    }
    break;
  case gamepad::axis::HAT2:
    switch (coord) {
    case x:
      res = {.code = ABS_HAT2X, .define_name = "ABS_HAT2X"sv};
      break;
    case y:
      res = {.code = ABS_HAT2Y, .define_name = "ABS_HAT2Y"sv};
      break;
    case z:
      return std::unexpected(error::no_z_for_2d_axis);
    }
    break;
  }
  return res;
}

constexpr std::expected<gamepad::key, evio::error> code2key(
    std::uint16_t code) noexcept {
  switch (code) {
  case BTN_A:
    return gamepad::key::A;
  case BTN_B:
    return gamepad::key::B;
  case BTN_X:
    return gamepad::key::X;
  case BTN_Y:
    return gamepad::key::Y;
  case BTN_TL:
    return gamepad::key::left_trg;
  case BTN_TR:
    return gamepad::key::right_trg;
  case BTN_TL2:
    return gamepad::key::left_alt_trg;
  case BTN_TR2:
    return gamepad::key::right_alt_trg;
  case BTN_DPAD_UP:
    return gamepad::key::dpad_up;
  case BTN_DPAD_DOWN:
    return gamepad::key::dpad_down;
  case BTN_DPAD_LEFT:
    return gamepad::key::dpad_left;
  case BTN_DPAD_RIGHT:
    return gamepad::key::dpad_right;
  case BTN_SELECT:
    return gamepad::key::select;
  case BTN_START:
    return gamepad::key::start;
  }
  return std::unexpected(error::unknown_key_code);
}

} // namespace evio
