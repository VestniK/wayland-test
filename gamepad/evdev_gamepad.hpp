#pragma once

#include <filesystem>

#include <asio/awaitable.hpp>
#include <asio/posix/stream_descriptor.hpp>
#include <asio/io_context.hpp>

class evdev_gamepad {
public:
  evdev_gamepad(asio::io_context::executor_type io_executor,
                const std::filesystem::path &devnode);

  evdev_gamepad(const evdev_gamepad&) = delete;
  evdev_gamepad(evdev_gamepad&&) = delete;
  evdev_gamepad& operator=(const evdev_gamepad&) = delete;
  evdev_gamepad& operator=(evdev_gamepad&&) = delete;
  ~evdev_gamepad() {dev_.cancel();}

private:
  asio::awaitable<void>
  watch_device(asio::io_context::executor_type io_executor,
               const std::filesystem::path &devnode);

private:
  asio::posix::stream_descriptor dev_;
};
