#include "evdev_gamepad.hpp"

#include <linux/input.h>

#include <asio/bind_cancellation_slot.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/posix/stream_descriptor.hpp>

#include <spdlog/spdlog.h>

#include <util/io.hpp>

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

  co_return;
}
