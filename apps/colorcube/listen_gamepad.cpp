#include "listen_gamepad.hpp"

#include <libs/gamepad/udev_gamepads.hpp>

asio::awaitable<void> listen_gamepad(co::io_executor io_exec, scene::controller& controller) {
  udev_gamepads gamepads{std::ref(controller), std::ref(controller), std::ref(controller)};
  co_await gamepads.watch(io_exec);
}
