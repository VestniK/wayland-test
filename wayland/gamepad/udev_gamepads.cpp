#include <memory>
#include <ranges>
#include <string_view>>
#include <system_error>

#include <libudev.h>

#include <spdlog/spdlog.h>

#include <asio/awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/posix/stream_descriptor.hpp>
#include <asio/use_awaitable.hpp>

#include "udev_gamepads.hpp"

using namespace std::literals;

namespace {

asio::awaitable<void>
monitor_loop(asio::io_context::executor_type exec,
             detail::udev::unique_ptr<udev_monitor> monitor) {
  while (true) {
    asio::posix::stream_descriptor conn{exec,
                                        udev_monitor_get_fd(monitor.get())};
    co_await conn.async_wait(asio::posix::stream_descriptor::wait_read,
                             asio::use_awaitable);
    conn.release();
    detail::udev::unique_ptr<udev_device> dev{
        udev_monitor_receive_device(monitor.get())};
    if (!dev) {
      continue;
    }
    const char *action = udev_device_get_action(dev.get());
    const char *devnode = udev_device_get_devnode(dev.get());
    if (devnode == nullptr)
      continue;

    const char *joy_prop =
        udev_device_get_property_value(dev.get(), "ID_INPUT_JOYSTICK");
    if (!joy_prop || "1"sv != joy_prop)
      continue;

    const char *sysname = udev_device_get_sysname(dev.get());
    const char *sysnum = udev_device_get_sysnum(dev.get());
    const char *syspath = udev_device_get_syspath(dev.get());

    spdlog::info("{} gamepad:\n\t{}\n\t{}\n  sysname: {}\n  sysnum: {}", action,
                 syspath, devnode, sysname, sysnum);
  }
  co_return;
}

} // namespace

udev_gamepads::connected_gamepads::connected_gamepads(udev &udev)
    : enumerator_{udev_enumerate_new(&udev)} {
  udev_enumerate_add_match_subsystem(enumerator_.get(), "input");
  udev_enumerate_add_match_property(enumerator_.get(), "ID_INPUT_JOYSTICK",
                                    "1");
  if (int ret = udev_enumerate_scan_devices(enumerator_.get()); ret < 0)
    throw std::system_error{-ret, std::system_category(),
                            "udev_enumerate_scan_devices"};
}

void udev_gamepads::watch(asio::io_context::executor_type exec) {
  detail::udev::unique_ptr<udev_monitor> monitor{
      udev_monitor_new_from_netlink(udev_.get(), "udev")};
  udev_monitor_filter_add_match_subsystem_devtype(monitor.get(), "input", 0);
  udev_monitor_enable_receiving(monitor.get());

  asio::co_spawn(
      exec, monitor_loop(exec, std::move(monitor)), [](std::exception_ptr err) {
        if (err) {
          try {
            std::rethrow_exception(err);
          } catch (const std::exception &err) {
            spdlog::error("Gamepads monitoring failure: {}", err.what());
          }
        }
      });
}

void udev_gamepads::list() {
  for (auto &entry : connected()) {
    const char *syspath = udev_list_entry_get_name(&entry);
    detail::udev::unique_ptr<udev_device> dev{
        udev_device_new_from_syspath(&native_handle(), syspath)};
    const char *devnode = udev_device_get_devnode(dev.get());
    if (devnode == nullptr)
      continue;

    const char *sysname = udev_device_get_sysname(dev.get());
    const char *sysnum = udev_device_get_sysnum(dev.get());

    spdlog::info("found gamepad:\n\t{}\n\t{}\n  sysname: {}\n  sysnum: {}",
                 syspath, devnode, sysname, sysnum);
  }
}
