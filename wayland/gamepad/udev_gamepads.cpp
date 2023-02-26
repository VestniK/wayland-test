#include <memory>
#include <ranges>
#include <string_view>
#include <system_error>

#include <libudev.h>

#include <spdlog/spdlog.h>

#include <asio/co_spawn.hpp>
#include <asio/posix/stream_descriptor.hpp>
#include <asio/use_awaitable.hpp>

#include "udev_gamepads.hpp"

using namespace std::literals;

namespace {

struct non_owning_stream_descriptor : asio::posix::stream_descriptor {
  using asio::posix::stream_descriptor::stream_descriptor;
  ~non_owning_stream_descriptor() noexcept { this->release(); }
};

constexpr auto nullsv = "nullptr"sv;
std::string_view as_sv(const char *badcstr) noexcept {
  return badcstr ? std::string_view{badcstr} : nullsv;
}

void log_gamepad_info(std::string_view action, std::string_view devnode,
                      udev_device &dev) {
  const char *sysname = udev_device_get_sysname(&dev);
  const char *sysnum = udev_device_get_sysnum(&dev);
  const char *syspath = udev_device_get_syspath(&dev);

  using detail::udev::list_range;
  spdlog::debug(
      "{} gamepad:\n\t{}\n\t{}\n  sysname: {}\n  sysnum: {}\n  "
      "devpath: {}\n  subsystem: {}\n  devtype: {}\n  props:\n\t{}",
      action, syspath, devnode, sysname, sysnum,
      as_sv(udev_device_get_devpath(&dev)),
      as_sv(udev_device_get_subsystem(&dev)),
      as_sv(udev_device_get_devtype(&dev)),
      fmt::join(list_range{udev_device_get_properties_list_entry(&dev)} |
                    std::views::transform([](udev_list_entry &entry) {
                      return fmt::format(
                          "{} = {}", as_sv(udev_list_entry_get_name(&entry)),
                          as_sv(udev_list_entry_get_value(&entry)));
                    }),
                "\n\t"));
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

asio::awaitable<void>
udev_gamepads::watch(asio::io_context::executor_type exec) {
  non_owning_stream_descriptor conn{exec, udev_monitor_get_fd(monitor_.get())};
  while (true) {
    co_await conn.async_wait(asio::posix::stream_descriptor::wait_read,
                             asio::use_awaitable);
    detail::udev::unique_ptr<udev_device> dev{
        udev_monitor_receive_device(monitor_.get())};
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

    log_gamepad_info(action, devnode, *dev);
  }
  co_return;
}

udev_gamepads::udev_gamepads() {
  udev_monitor_filter_add_match_subsystem_devtype(monitor_.get(), "input", 0);
  udev_monitor_enable_receiving(monitor_.get());
}

void udev_gamepads::list() {
  for (auto &entry : connected()) {
    const char *syspath = udev_list_entry_get_name(&entry);
    detail::udev::unique_ptr<udev_device> dev{
        udev_device_new_from_syspath(&native_handle(), syspath)};
    const char *devnode = udev_device_get_devnode(dev.get());
    if (devnode == nullptr)
      continue;

    log_gamepad_info("found", devnode, *dev);
  }
}

static_assert(std::input_iterator<detail::udev::list_iterator>);
static_assert(std::sentinel_for<detail::udev::list_iterator,
                                detail::udev::list_iterator>);
static_assert(std::ranges::input_range<udev_gamepads::connected_gamepads>);
static_assert(std::ranges::input_range<detail::udev::list_range>);
