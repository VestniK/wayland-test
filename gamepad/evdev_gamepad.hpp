#pragma once

#include <filesystem>

#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>
#include <asio/posix/stream_descriptor.hpp>

enum class gamepad_key {
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

class evdev_gamepad {
public:
  using key_handler = std::function<void(gamepad_key, bool)>;
  evdev_gamepad(asio::io_context::executor_type io_executor,
      const std::filesystem::path& devnode);

  evdev_gamepad(const evdev_gamepad&) = delete;
  evdev_gamepad(evdev_gamepad&&) = delete;
  evdev_gamepad& operator=(const evdev_gamepad&) = delete;
  evdev_gamepad& operator=(evdev_gamepad&&) = delete;
  ~evdev_gamepad() { dev_.cancel(); }

  void set_key_handler(const key_handler& val) { key_handler_ = val; }
  void set_key_handler(key_handler&& val) { key_handler_ = std::move(val); }

private:
  asio::awaitable<void> watch_device(
      asio::io_context::executor_type io_executor,
      const std::filesystem::path& devnode);

private:
  asio::posix::stream_descriptor dev_;
  key_handler key_handler_;
};
