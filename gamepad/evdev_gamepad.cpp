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

} // namespace

evdev_gamepad::evdev_gamepad(asio::io_context::executor_type io_executor,
                             const std::filesystem::path &devnode) {
  asio::co_spawn(
      io_executor, watch_device(io_executor, devnode),
      asio::bind_cancellation_slot(stop_sig_.slot(), asio::detached));
}

asio::awaitable<void>
evdev_gamepad::watch_device(asio::io_context::executor_type io_executor,
                            const std::filesystem::path &devnode) {
  asio::posix::stream_descriptor conn{
      io_executor,
      io::open(devnode.c_str(), io::mode::read_only | io::mode::ndelay)
          .release()};

  input_id id;
  if (ioctl(conn.native_handle(), EVIOCGID, &id) == -1)
    throw std::system_error{errno, std::system_category(), "ioctl(EVIOCGID)"};
  spdlog::debug("gamepad id: bustype[{}]-vendor[{}]-product[{}]-version[{}]",
                id.bustype, id.vendor, id.product, id.version);

  std::array<char, 256> namebuf;
  if (ioctl(conn.native_handle(), EVIOCGNAME(namebuf.size() - 1),
            namebuf.data()) == -1)
    throw std::system_error{errno, std::system_category(), "ioctl(EVIOCGNAME)"};
  spdlog::debug("gamepad name: {}", namebuf.data());

  std::array<input_event, 32> events;
  while (true) {
    size_t bytes_read = 0;
    while (bytes_read == 0 || bytes_read % sizeof(input_event) != 0) {
      // TODO handle EOF properly
      bytes_read += co_await conn.async_read_some(
          span2buf(
              std::as_writable_bytes(std::span{events}).subspan(bytes_read)),
          asio::use_awaitable);
    }
    const std::span<const input_event> events_read =
        std::span{events}.subspan(bytes_read / sizeof(input_event));
    for (const auto &ev : events_read) {
      switch (ev.type) {
      case EV_KEY:
        spdlog::debug("key even: time: {} code: {} value: {}",
                      std::chrono::seconds{ev.time.tv_sec} +
                          std::chrono::microseconds{ev.time.tv_usec},
                      ev.code, ev.value);
      case EV_ABS:
        spdlog::debug("abs event: time: {} code: {} value: {}",
                      std::chrono::seconds{ev.time.tv_sec} +
                          std::chrono::microseconds{ev.time.tv_usec},
                      ev.code, ev.value);
      }
    }
  }

  co_return;
}
