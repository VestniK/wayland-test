#pragma once

#include <filesystem>

#include <asio/awaitable.hpp>
#include <asio/cancellation_signal.hpp>
#include <asio/io_context.hpp>

class evdev_gamepad {
public:
  evdev_gamepad(asio::io_context::executor_type io_executor,
                const std::filesystem::path &devnode);

  ~evdev_gamepad() noexcept {
    stop_sig_.emit(asio::cancellation_type_t::terminal);
  }
  evdev_gamepad(evdev_gamepad &&) noexcept = delete;
  evdev_gamepad &operator=(evdev_gamepad &&) noexcept = delete;
  evdev_gamepad(const evdev_gamepad &) = delete;
  evdev_gamepad &operator=(const evdev_gamepad &) = delete;

private:
  asio::awaitable<void>
  watch_device(asio::io_context::executor_type io_executor,
               const std::filesystem::path &devnode);

private:
  asio::cancellation_signal stop_sig_;
};
