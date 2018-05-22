#pragma once

#include <portable_concurrency/future>

#include <fmt/format.h>

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

#include <wayland/client.hpp>

#include "xkb.hpp"

using namespace fmt::literals;

wl::keycode keycode_conv(xkb::keycode val) noexcept {
  return wl::keycode{ut::underlying_cast(val) - 8};
}

xkb::keycode keycode_conv(wl::keycode val) noexcept {
  return xkb::keycode{ut::underlying_cast(val) + 8};
}

class quit_waiter {
public:
  void set_seat(wl::seat seat) {
    seat_ = std::move(seat);
    LOG4CPLUS_DEBUG(log, "Seat version: {}"_format(seat_.get_version()));
    seat_.add_listener(*this);
  }

  pc::future<void> get_quit_future() {return promise.get_future();}

  void keymap(wl::keyboard::ref, wl::keyboard::keymap_format fmt, int fd, size_t size) try {
    LOG4CPLUS_DEBUG(log, "Received keymap. Format: {}; of size: {}"_format(fmt, size));
    if (fmt != wl::keyboard::keymap_format::xkb_v1)
      throw std::runtime_error{"Unsupported keymap format"};

    io::shared_memory shmem{io::file_descriptor{fd}, size};
    xkb::context ctx;
    kmap = ctx.load_keyap(shmem.data(), shmem.size() - 1);
    const xkb::keycode esc_xkb = kmap.key_by_name("ESC");
    if (esc_xkb == xkb::keycode::invalid)
      throw std::runtime_error{"failed to find Escape key code"};
    esc_keycode = keycode_conv(esc_xkb);
    LOG4CPLUS_DEBUG(log, "Escape keycode: {}"_format(ut::underlying_cast(esc_keycode)));
  } catch(...) {
    promise.set_exception(std::current_exception());
  }

  void enter(wl::keyboard::ref, wl::serial eid, wl::surface::ref, wl_array*) {
    LOG4CPLUS_DEBUG(log, "Surface get focus[{}]"_format(eid));
  }
  void leave(wl::keyboard::ref, wl::serial eid, wl::surface::ref) {
    LOG4CPLUS_DEBUG(log, "Surface lost focus[{}]"_format(eid));
  }
  void key(wl::keyboard::ref, wl::serial eid, wl::clock::time_point time, wl::keycode key, wl::keyboard::key_state state) {
    LOG4CPLUS_DEBUG(log, "Key event[{}] timestamp: {}; key code: {} [{}]; key state: {}"_format(
      eid, time.time_since_epoch().count(), ut::underlying_cast(key), kmap.key_get_name(keycode_conv(key)), state
    ));

    if (key == esc_keycode && state == wl::keyboard::key_state::pressed) {
      promise.set_value();
      return;
    }
  }
  void modifiers(
    wl::keyboard::ref, wl::serial eid, uint32_t mods_depressed, uint32_t mods_latched,
    uint32_t mods_locked, uint32_t group
  ) {
    LOG4CPLUS_DEBUG(log, "Modifiers event[{}] mds_depresed: {}; mods latched: {}; mods locked: {}; group: {}"_format(
      eid, mods_depressed, mods_latched, mods_locked, group
    ));
  }
  void repeat_info(wl::keyboard::ref, int32_t rate, std::chrono::milliseconds delay) {
    LOG4CPLUS_DEBUG(log, "Repeat info: rate: {}; delay: {}ms"_format(rate, delay.count()));
  }

  void capabilities(wl::seat::ref seat, ut::bitmask<wl::seat::capability> caps) {
    LOG4CPLUS_DEBUG(log, "Seat capabilities: {}"_format(caps.value()));
    if (!keyboard_ && (caps & wl::seat::capability::keyboard)) {
      keyboard_ = seat.get_keyboard();
      LOG4CPLUS_DEBUG(log, "Found keyboard of version: {}"_format(keyboard_.get_version()));
      keyboard_.add_listener(*this);
    }
    if (keyboard_ && !(caps & wl::seat::capability::keyboard)) {
      LOG4CPLUS_DEBUG(log, "Keyboard disappeared");
      keyboard_ = {};
    }
  }

  void name(wl::seat::ref, const char* seat_name) {
    LOG4CPLUS_DEBUG(log, "Seat name: {}"_format(seat_name));
  }

private:
  log4cplus::Logger log = log4cplus::Logger::getInstance("INPUT");
  wl::seat seat_;
  wl::keyboard keyboard_;
  pc::promise<void> promise;
  xkb::keymap kmap;
  wl::keycode esc_keycode = {};
};
