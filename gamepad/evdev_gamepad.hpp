#pragma once

#include <array>
#include <filesystem>
#include <type_traits>

#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>
#include <asio/posix/stream_descriptor.hpp>

#include <glm/vec2.hpp>

#include <util/channel.hpp>

#include <gamepad/types/axis.hpp>
#include <gamepad/types/axis_state.hpp>
#include <gamepad/types/kyes.hpp>

class evdev_gamepad {
public:
  using key_handler = std::function<void(gamepad::key, bool)>;
  using axis2d_handler =
      std::function<void(gamepad::axis, gamepad::axis2d_state)>;
  using axis3d_handler =
      std::function<void(gamepad::axis, gamepad::axis3d_state)>;
  evdev_gamepad(asio::io_context::executor_type io_executor,
      const std::filesystem::path& devnode);

  evdev_gamepad(const evdev_gamepad&) = delete;
  evdev_gamepad(evdev_gamepad&&) = delete;
  evdev_gamepad& operator=(const evdev_gamepad&) = delete;
  evdev_gamepad& operator=(evdev_gamepad&&) = delete;
  ~evdev_gamepad() { dev_.cancel(); }

  void set_key_handler(const key_handler& val) { key_handler_ = val; }
  void set_key_handler(key_handler&& val) { key_handler_ = std::move(val); }

  void set_axis_handler(const axis2d_handler& val) { axis2d_handler_ = val; }
  void set_axis_handler(axis2d_handler&& val) {
    axis2d_handler_ = std::move(val);
  }

  void set_axis_handler(const axis3d_handler& val) { axis3d_handler_ = val; }
  void set_axis_handler(axis3d_handler&& val) {
    axis3d_handler_ = std::move(val);
  }

private:
  asio::awaitable<void> watch_device(
      asio::io_context::executor_type io_executor);

private:
  asio::posix::stream_descriptor dev_;
  key_handler key_handler_;
  axis2d_handler axis2d_handler_;
  axis3d_handler axis3d_handler_;
};
