#include <memory>
#include <ranges>
#include <string_view>
#include <system_error>

#include <libudev.h>

#include <fmt/ranges.h>

#include <spdlog/spdlog.h>

#include <asio/co_spawn.hpp>
#include <asio/posix/stream_descriptor.hpp>
#include <asio/use_awaitable.hpp>

#include "udev_gamepads.hpp"

using namespace std::literals;

namespace {

namespace notify {

constexpr auto add = "add"sv;
constexpr auto remove = "remove"sv;

} // namespace notify

struct non_owning_stream_descriptor : asio::posix::stream_descriptor {
  using asio::posix::stream_descriptor::stream_descriptor;
  ~non_owning_stream_descriptor() noexcept { this->release(); }
};

constexpr auto nullsv = "nullptr"sv;
std::string_view as_sv(const char* badcstr) noexcept { return badcstr ? std::string_view{badcstr} : nullsv; }

void log_gamepad_info(std::string_view action, std::string_view devnode, udev_device& dev) {
  const char* sysnum = udev_device_get_sysnum(&dev);
  const char* syspath = udev_device_get_syspath(&dev);

  using detail::udev::list_range;
  spdlog::debug(
      "{} gamepad:\n\t{}\n\t{}\n  sysnum: {}\n  "
      "devpath: {}\n  subsystem: {}\n  devtype: {}\n  props:\n\t{}",
      action, syspath, devnode, sysnum, as_sv(udev_device_get_devpath(&dev)),
      as_sv(udev_device_get_subsystem(&dev)), as_sv(udev_device_get_devtype(&dev)),
      fmt::join(
          list_range{udev_device_get_properties_list_entry(&dev)
          } | std::views::transform([](udev_list_entry& entry) {
            return fmt::format(
                "{} = {}", as_sv(udev_list_entry_get_name(&entry)), as_sv(udev_list_entry_get_value(&entry))
            );
          }),
          "\n\t"
      )
  );
}

bool is_evdev_devnode(const char* devnode, udev_device& dev) noexcept {
  const auto sysname = as_sv(udev_device_get_sysname(&dev));
  return devnode && std::string_view{sysname}.starts_with("event");
}

} // namespace

udev_gamepads::connected_gamepads::connected_gamepads(udev& udev) : enumerator_{udev_enumerate_new(&udev)} {
  udev_enumerate_add_match_subsystem(enumerator_.get(), "input");
  udev_enumerate_add_match_property(enumerator_.get(), "ID_INPUT_JOYSTICK", "1");
  if (int ret = udev_enumerate_scan_devices(enumerator_.get()); ret < 0)
    throw std::system_error{-ret, std::system_category(), "udev_enumerate_scan_devices"};
}

asio::awaitable<void> udev_gamepads::watch(asio::io_context::executor_type exec) {
  for (auto [devnode, dev] :
       connected() | std::views::transform([&](udev_list_entry& entry) {
         const char* syspath = udev_list_entry_get_name(&entry);
         detail::udev::unique_ptr<udev_device> dev{udev_device_new_from_syspath(&native_handle(), syspath)};
         const char* devnode = udev_device_get_devnode(dev.get());
         return std::tuple{devnode, std::move(dev)};
       }) | std::views::filter([](const auto& devinfo) {
         const auto& [devnode, dev] = devinfo;
         return is_evdev_devnode(devnode, *dev);
       })) {
    log_gamepad_info("found", devnode, *dev);
    on_add(exec, devnode, *dev);
  }

  non_owning_stream_descriptor conn{exec, udev_monitor_get_fd(monitor_.get())};
  while (true) {
    co_await conn.async_wait(asio::posix::stream_descriptor::wait_read, asio::use_awaitable);
    detail::udev::unique_ptr<udev_device> dev{udev_monitor_receive_device(monitor_.get())};
    if (!dev) {
      continue;
    }
    const char* action = udev_device_get_action(dev.get());
    const char* devnode = udev_device_get_devnode(dev.get());
    if (!is_evdev_devnode(devnode, *dev))
      continue;

    const char* joy_prop = udev_device_get_property_value(dev.get(), "ID_INPUT_JOYSTICK");
    if (!joy_prop || "1"sv != joy_prop)
      continue;

    log_gamepad_info(action, devnode, *dev);
    if (action == notify::add)
      on_add(exec, devnode, *dev);
    else if (action == notify::remove)
      on_remove(devnode, *dev);
    else
      spdlog::error("Unknown device action {}", action);
  }
  co_return;
}

udev_gamepads::udev_gamepads(
    evdev_gamepad::key_handler key_handler, evdev_gamepad::axis2d_handler axis2d_handler,
    evdev_gamepad::axis3d_handler axis3d_handler
)
    : key_handler_{key_handler}, axis2d_handler_{axis2d_handler}, axis3d_handler_{axis3d_handler} {
  udev_monitor_filter_add_match_subsystem_devtype(monitor_.get(), "input", 0);
  udev_monitor_enable_receiving(monitor_.get());
}

void udev_gamepads::on_add(asio::io_context::executor_type exec, std::string_view devnode, udev_device& dev) {
  auto [it, inserted] = gamepads_.try_emplace(devnode, exec, devnode);
  if (!inserted) {
    spdlog::warn(
        "Gamepad for devnode {} is already connected while '{}' "
        "notification received",
        devnode, notify::add
    );
  }
  it->second.set_key_handler(key_handler_);
  it->second.set_axis_handler(axis2d_handler_);
  it->second.set_axis_handler(axis3d_handler_);
}
void udev_gamepads::on_remove(std::string_view devnode, udev_device& dev) {
  auto it = gamepads_.find(devnode);
  if (it == gamepads_.end()) {
    spdlog::warn(
        "Gamepad for devnode {} is not connected while '{}' "
        "notification received",
        devnode, notify::remove
    );
    return;
  }
  gamepads_.erase(it);
}

static_assert(std::input_iterator<detail::udev::list_iterator>);
static_assert(std::sentinel_for<detail::udev::list_iterator, detail::udev::list_iterator>);
static_assert(std::ranges::input_range<udev_gamepads::connected_gamepads>);
static_assert(std::ranges::input_range<detail::udev::list_range>);
