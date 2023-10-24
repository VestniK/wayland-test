#include "evdev_gamepad.hpp"

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

namespace {

template <size_t Extent = std::dynamic_extent>
auto span2buf(std::span<std::byte, Extent> in) {
  return asio::buffer(in.data(), in.size());
}

[[maybe_unused]] std::string_view ev_type_name(std::uint16_t type) noexcept {
  switch (type) {
  case EV_SYN:
    return "EV_SYN"sv;
  case EV_KEY:
    return "EV_KEY"sv;
  case EV_REL:
    return "EV_REL"sv;
  case EV_ABS:
    return "EV_ABS"sv;
  case EV_MSC:
    return "EV_MSC"sv;
  case EV_SW:
    return "EV_SW"sv;
  case EV_LED:
    return "EV_LED"sv;
  case EV_SND:
    return "EV_SND"sv;
  case EV_REP:
    return "EV_REP"sv;
  case EV_FF:
    return "EV_FF"sv;
  case EV_PWR:
    return "EV_PWR"sv;
  case EV_FF_STATUS:
    return "EV_FF_STATUS"sv;
  }
  return "unknown evdev event type"sv;
}

auto evcode2axis(uint16_t evcode) noexcept {
  struct {
    gamepad::axis axis;
    int coord;
  } res;
  switch (evcode) {
  case ABS_X:
    res.axis = gamepad::axis::main;
    res.coord = 0;
    break;
  case ABS_Y:
    res.axis = gamepad::axis::main;
    res.coord = 1;
    break;
  case ABS_RX:
    res.axis = gamepad::axis::rotational;
    res.coord = 0;
    break;
  case ABS_RY:
    res.axis = gamepad::axis::rotational;
    res.coord = 1;
    break;
  case ABS_HAT0X:
    res.axis = gamepad::axis::HAT0;
    res.coord = 0;
    break;
  case ABS_HAT0Y:
    res.axis = gamepad::axis::HAT0;
    res.coord = 1;
    break;
  case ABS_HAT2X:
    res.axis = gamepad::axis::HAT2;
    res.coord = 0;
    break;
  case ABS_HAT2Y:
    res.axis = gamepad::axis::HAT2;
    res.coord = 1;
    break;
  }
  return res;
}

namespace evio {
enum class axe_error { z_dim_requested_for_2d_axis };
struct axe {
  uint16_t code;
  std::string_view define_name;
};
} // namespace evio

constexpr std::expected<evio::axe, evio::axe_error> axis2evcode(
    gamepad::axis axis, gamepad::dimention coord) noexcept {
  using enum gamepad::dimention;
  evio::axe res;
  switch (axis) {
  case gamepad::axis::main:
    switch (coord) {
    case x:
      res = {.code = ABS_X, .define_name = "ABS_X"sv};
      break;
    case y:
      res = {.code = ABS_Y, .define_name = "ABS_Y"sv};
      break;
    case z:
      res = {.code = ABS_Z, .define_name = "ABS_Z"sv};
      break;
    }
    break;
  case gamepad::axis::rotational:
    switch (coord) {
    case x:
      res = {.code = ABS_RX, .define_name = "ABS_RX"sv};
      break;
    case y:
      res = {.code = ABS_RY, .define_name = "ABS_RY"sv};
      break;
    case z:
      res = {.code = ABS_RZ, .define_name = "ABS_RZ"sv};
      break;
    }
    break;
  case gamepad::axis::HAT0:
    switch (coord) {
    case x:
      res = {.code = ABS_HAT0X, .define_name = "ABS_HAT0X"sv};
      break;
    case y:
      res = {.code = ABS_HAT0Y, .define_name = "ABS_HAT0Y"sv};
      break;
    case z:
      return std::unexpected(evio::axe_error::z_dim_requested_for_2d_axis);
    }
    break;
  case gamepad::axis::HAT2:
    switch (coord) {
    case x:
      res = {.code = ABS_HAT2X, .define_name = "ABS_HAT2X"sv};
      break;
    case y:
      res = {.code = ABS_HAT2Y, .define_name = "ABS_HAT2Y"sv};
      break;
    case z:
      return std::unexpected(evio::axe_error::z_dim_requested_for_2d_axis);
    }
    break;
  }
  return res;
}

std::expected<input_absinfo, std::error_code> load_absinfo(
    asio::posix::stream_descriptor& dev, uint16_t code) {
  input_absinfo axis_limits{};
  if (ioctl(dev.native_handle(), EVIOCGABS(code), &axis_limits) == -1)
    return std::unexpected(std::error_code{errno, std::system_category()});
  return axis_limits;
}

std::expected<input_absinfo, evio::axe_error> load_absinfo(
    asio::posix::stream_descriptor& dev, gamepad::axis axis,
    gamepad::dimention coord) {
  return axis2evcode(axis, coord).transform([&](auto axe) {
    return load_absinfo(dev, axe.code)
        .or_else([&](auto ec) -> std::expected<input_absinfo, std::error_code> {
          throw std::system_error{
              ec, fmt::format("ioctl(EVIOCGNAME({}))", axe.define_name)};
        })
        .value();
  });
}

} // namespace

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

  load_absinfo(dev_, gamepad::axis::main, gamepad::dimention::x);
  load_absinfo(dev_, gamepad::axis::main, gamepad::dimention::y);
  load_absinfo(dev_, gamepad::axis::rotational, gamepad::dimention::x);
  load_absinfo(dev_, gamepad::axis::rotational, gamepad::dimention::y);
  load_absinfo(dev_, gamepad::axis::HAT0, gamepad::dimention::x);
  load_absinfo(dev_, gamepad::axis::HAT0, gamepad::dimention::y);
  load_absinfo(dev_, gamepad::axis::HAT2, gamepad::dimention::x);
  load_absinfo(dev_, gamepad::axis::HAT2, gamepad::dimention::y);

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
      if (!key_handler_)
        break;
      switch (ev.type) {
      case EV_KEY:
        switch (ev.code) {
        case BTN_A:
          key_handler_(gamepad::key::A, ev.value != 0);
          break;
        case BTN_B:
          key_handler_(gamepad::key::B, ev.value != 0);
          break;
        case BTN_X:
          key_handler_(gamepad::key::X, ev.value != 0);
          break;
        case BTN_Y:
          key_handler_(gamepad::key::Y, ev.value != 0);
          break;
        case BTN_TL:
          key_handler_(gamepad::key::left_trg, ev.value != 0);
          break;
        case BTN_TR:
          key_handler_(gamepad::key::right_trg, ev.value != 0);
          break;
        case BTN_TL2:
          key_handler_(gamepad::key::left_alt_trg, ev.value != 0);
          break;
        case BTN_TR2:
          key_handler_(gamepad::key::right_alt_trg, ev.value != 0);
          break;
        case BTN_DPAD_UP:
          key_handler_(gamepad::key::dpad_up, ev.value != 0);
          break;
        case BTN_DPAD_DOWN:
          key_handler_(gamepad::key::dpad_down, ev.value != 0);
          break;
        case BTN_DPAD_LEFT:
          key_handler_(gamepad::key::dpad_left, ev.value != 0);
          break;
        case BTN_DPAD_RIGHT:
          key_handler_(gamepad::key::dpad_right, ev.value != 0);
          break;
        case BTN_SELECT:
          key_handler_(gamepad::key::select, ev.value != 0);
          break;
        case BTN_START:
          key_handler_(gamepad::key::start, ev.value != 0);
          break;
        }
        break;
      case EV_ABS:
        if (axis_state_) {
          const auto axisinf = evcode2axis(ev.code);
          auto& state = axis_state_->get(axisinf.axis);
          state.current[axisinf.coord] = ev.value;
          if (state.channel)
            state.channel->update(state.current);
        }
        break;
      }
    }
  }

  co_return;
}
