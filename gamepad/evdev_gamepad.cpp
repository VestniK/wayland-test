#include "evdev_gamepad.hpp"

#include <linux/input.h>

#include <asio/bind_cancellation_slot.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/posix/stream_descriptor.hpp>

#include <fmt/chrono.h>
#include <spdlog/spdlog.h>

#include <util/io.hpp>

namespace {

template <size_t Extent = std::dynamic_extent>
auto span2buf(std::span<std::byte, Extent> in) {
  return asio::buffer(in.data(), in.size());
}

std::string_view ev_type_name(std::uint16_t type) noexcept {
  switch (type) {
  case EV_SYN:
    return "EV_SYN";
  case EV_KEY:
    return "EV_KEY";
  case EV_REL:
    return "EV_REL";
  case EV_ABS:
    return "EV_ABS";
  case EV_MSC:
    return "EV_MSC";
  case EV_SW:
    return "EV_SW";
  case EV_LED:
    return "EV_LED";
  case EV_SND:
    return "EV_SND";
  case EV_REP:
    return "EV_REP";
  case EV_FF:
    return "EV_FF";
  case EV_PWR:
    return "EV_PWR";
  case EV_FF_STATUS:
    return "EV_FF_STATUS";
  }
  return "unknown evdev event type";
}

} // namespace

evdev_gamepad::evdev_gamepad(asio::io_context::executor_type io_executor,
    const std::filesystem::path& devnode)
    : dev_{io_executor,
          io::open(devnode, io::mode::read_only | io::mode::ndelay).release()} {
  asio::co_spawn(
      io_executor, watch_device(io_executor, devnode), asio::detached);
}

asio::awaitable<void> evdev_gamepad::watch_device(
    asio::io_context::executor_type io_executor,
    const std::filesystem::path& devnode) {
  input_id id;
  if (ioctl(dev_.native_handle(), EVIOCGID, &id) == -1)
    throw std::system_error{errno, std::system_category(), "ioctl(EVIOCGID)"};
  spdlog::debug("gamepad id: bustype[{}]-vendor[{}]-product[{}]-version[{}]",
      id.bustype, id.vendor, id.product, id.version);

  std::array<char, 256> namebuf;
  if (ioctl(dev_.native_handle(), EVIOCGNAME(namebuf.size() - 1),
          namebuf.data()) == -1)
    throw std::system_error{errno, std::system_category(), "ioctl(EVIOCGNAME)"};
  spdlog::debug("gamepad name: {}", namebuf.data());

  std::array<input_event, 32> events;
  while (true) {
    size_t bytes_read = 0;
    while (bytes_read == 0 || bytes_read % sizeof(input_event) != 0) {
      // TODO handle EOF properly
      bytes_read += co_await dev_.async_read_some(
          span2buf(
              std::as_writable_bytes(std::span{events}).subspan(bytes_read)),
          asio::use_awaitable);
    }
    const std::span<const input_event> events_read =
        std::span{events}.subspan(0, bytes_read / sizeof(input_event));
    for (const auto& ev : events_read) {
      if (ev.type == EV_SYN)
        continue;
      SPDLOG_TRACE("{}: time: {} code: 0x{:x} value: {}", ev_type_name(ev.type),
          std::chrono::seconds{ev.time.tv_sec} +
              std::chrono::microseconds{ev.time.tv_usec},
          ev.code, ev.value);
      switch (ev.type) {
      case EV_KEY:
        break;
      case EV_ABS:
        break;
      }
    }
  }

  co_return;
}
