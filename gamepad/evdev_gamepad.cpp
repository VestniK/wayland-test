#include "evdev_gamepad.hpp"
#include "evio.hpp"
#include "evio_codes.hpp"

#include <expected>

#include <linux/input.h>

#include <asio/bind_cancellation_slot.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/posix/stream_descriptor.hpp>

#include <fmt/chrono.h>
#include <spdlog/spdlog.h>

#include <thinsys/io/io.hpp>

using namespace std::literals;

evdev_gamepad::evdev_gamepad(asio::io_context::executor_type io_executor,
    const std::filesystem::path& devnode)
    : dev_{io_executor,
          thinsys::io::open(
              devnode, thinsys::io::mode::read_only | thinsys::io::mode::ndelay)
              .release()} {
  asio::co_spawn(io_executor, watch_device(io_executor), asio::detached);
}

asio::awaitable<void> evdev_gamepad::watch_device(
    asio::io_context::executor_type io_executor) {
  const input_id id = evio::load_dev_id(dev_);
  spdlog::debug("gamepad id: bustype[{}]-vendor[{}]-product[{}]-version[{}]",
      id.bustype, id.vendor, id.product, id.version);
  spdlog::debug("gamepad name: {}", evio::load_dev_name(dev_));

  evio::load_absinfo(dev_, gamepad::axis::main, gamepad::dimention::x);
  evio::load_absinfo(dev_, gamepad::axis::main, gamepad::dimention::y);
  evio::load_absinfo(dev_, gamepad::axis::rotational, gamepad::dimention::x);
  evio::load_absinfo(dev_, gamepad::axis::rotational, gamepad::dimention::y);
  evio::load_absinfo(dev_, gamepad::axis::HAT0, gamepad::dimention::x);
  evio::load_absinfo(dev_, gamepad::axis::HAT0, gamepad::dimention::y);
  evio::load_absinfo(dev_, gamepad::axis::HAT2, gamepad::dimention::x);
  evio::load_absinfo(dev_, gamepad::axis::HAT2, gamepad::dimention::y);

  auto events = evio::read_events(dev_);
  while (const auto chunk = co_await events.async_resume(asio::use_awaitable)) {
    for (const input_event& ev : *chunk) {
      if (ev.type == EV_SYN)
        continue;
      SPDLOG_TRACE("{}: time: {} code: 0x{:x} value: {}",
          evio::ev_type_name(ev.type),
          std::chrono::seconds{ev.time.tv_sec} +
              std::chrono::microseconds{ev.time.tv_usec},
          ev.code, ev.value);
      if (!key_handler_)
        break;
      switch (ev.type) {
      case EV_KEY:
        if (auto key = evio::code2key(ev.code))
          key_handler_(key.value(), ev.value != 0);
      case EV_ABS:
        if (axis_state_) {
          const auto axisinf = evio::evcode2axis(ev.code);
          auto& state = axis_state_->get(axisinf.axis);
          state.current[std::to_underlying(axisinf.dim)] = ev.value;
          if (state.channel)
            state.channel->update(state.current);
        }
        break;
      }
    }
  }

  co_return;
}
