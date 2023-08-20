#pragma once

#include <array>
#include <filesystem>
#include <type_traits>

#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>
#include <asio/posix/stream_descriptor.hpp>

#include <glm/vec2.hpp>

#include <util/channel.hpp>

namespace gamepad {

enum class key {
  A,
  B,
  X,
  Y,

  left_trg,
  right_trg,
  left_alt_trg,
  right_alt_trg,

  dpad_up,
  dpad_down,
  dpad_left,
  dpad_right,

  select,
  start
};

enum class axis {
  main,
  rotational,
  HAT0,
  HAT2,
};

} // namespace gamepad

namespace detail {

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

} // namespace detail

class evdev_gamepad {
public:
  using key_handler = std::function<void(gamepad::key, bool)>;
  using axes_state = detail::axes_state;
  evdev_gamepad(asio::io_context::executor_type io_executor,
      const std::filesystem::path& devnode);

  evdev_gamepad(const evdev_gamepad&) = delete;
  evdev_gamepad(evdev_gamepad&&) = delete;
  evdev_gamepad& operator=(const evdev_gamepad&) = delete;
  evdev_gamepad& operator=(evdev_gamepad&&) = delete;
  ~evdev_gamepad() { dev_.cancel(); }

  void set_key_handler(const key_handler& val) { key_handler_ = val; }
  void set_key_handler(key_handler&& val) { key_handler_ = std::move(val); }
  void set_axis_state(axes_state& state) noexcept { axis_state_ = &state; }

private:
  asio::awaitable<void> watch_device(
      asio::io_context::executor_type io_executor);

private:
  asio::posix::stream_descriptor dev_;
  key_handler key_handler_;
  axes_state* axis_state_ = nullptr;
};
