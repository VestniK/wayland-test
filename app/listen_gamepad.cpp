#include <app/listen_gamepad.hpp>

#include <gamepad/udev_gamepads.hpp>

asio::awaitable<void> listen_gamepad(
    asio::io_context::executor_type io_exec, scene::controller& controller) {
  udev_gamepads gamepads{
      std::ref(controller), std::ref(controller), std::ref(controller)};
  co_await gamepads.watch(io_exec);
}
