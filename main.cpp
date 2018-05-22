#include <array>
#include <iostream>

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/configurator.h>
#include <log4cplus/initializer.h>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <portable_concurrency/future>

#include "io/file.hpp"
#include "io/shmem.hpp"

#include "wayland/client.hpp"

#include "registry_searcher.hpp"
#include "xkb.hpp"

using namespace std::literals;
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

class window {
public:
  window(wl::size size):
    size_{size},
    buffers_memory_{static_cast<size_t>(4*size.width*size.height)}
  {}

  void show(wl::compositor& compositor, wl::shell& shell, wl::shm& shm) {
    surface_ = compositor.create_surface();
    LOG4CPLUS_DEBUG(log_, "Created a surface of version: {}"_format(surface_.get_version()));
    sh_surf_ = shell.get_shell_surface(surface_);
    LOG4CPLUS_DEBUG(log_, "Created a shell surface of version: {}"_format(sh_surf_.get_version()));
    sh_surf_.add_listener(*this);
    sh_surf_.set_toplevel();

    wl::shm::pool pool = shm.create_pool(buffers_memory_.fd().native_handle(), buffers_memory_.size());
    LOG4CPLUS_DEBUG(log_, "Created a wl::shm::pool of version: {}"_format(pool.get_version()));
    buffer_ = pool.create_buffer(0, size_, 4*size_.width, wl::shm::format::ARGB8888);
    LOG4CPLUS_DEBUG(log_, "Created a wl::buffer of version: {}"_format(buffer_.get_version()));
    std::fill(buffers_memory_.data(), buffers_memory_.data() + buffers_memory_.size(), std::byte{0x88});
    surface_.attach(buffer_);
    surface_.commit();
  }

  void ping(wl::shell_surface::ref surf, wl::serial serial) {
    LOG4CPLUS_DEBUG(log_, "ping: {}"_format(serial));
    surf.pong(serial);
  }
  void configure(wl::shell_surface::ref, uint32_t edges, wl::size sz) {
    LOG4CPLUS_DEBUG(log_, "shell_surface cofiguration: edges: {}; size: {}x{}"_format(edges, sz.width, sz.height));
  }
  void popup_done(wl::shell_surface::ref) {
    LOG4CPLUS_DEBUG(log_, "popup done");
  }

private:
  log4cplus::Logger log_ = log4cplus::Logger::getInstance("UI");
  wl::size size_;
  wl::surface surface_;
  wl::shell_surface sh_surf_;
  io::shared_memory buffers_memory_;
  wl::buffer buffer_;
};

class ui_application {
public:
  pc::future<int> start(wl::compositor compositor, wl::shell shell, wl::shm shm, wl::seat seat) {
    compositor_ = std::move(compositor);
    shell_ = std::move(shell);
    shm_ = std::move(shm);
    quit_waiter_.set_seat(std::move(seat));

    LOG4CPLUS_DEBUG(log_, "Compositor version: {}"_format(compositor_.get_version()));
    LOG4CPLUS_DEBUG(log_, "Shell version: {}"_format(shell_.get_version()));
    LOG4CPLUS_DEBUG(log_, "SHM version: {}"_format(shm_.get_version()));

    shm_.add_listener(*this);

    wnd_.show(compositor_, shell_, shm_);

    std::cout << "==== Press Esc to quit ====\n";
    return quit_waiter_.get_quit_future().next([] {return EXIT_SUCCESS;});
  }

  void format(wl::shm::ref, wl::shm::format fmt) {
    LOG4CPLUS_DEBUG(log_, "supported pixel format code: {:x}"_format(ut::underlying_cast(fmt)));
  }

private:
  log4cplus::Logger log_ = log4cplus::Logger::getInstance("UI");
  wl::compositor compositor_;
  wl::shell shell_;
  wl::shm shm_;
  window wnd_{wl::size{240, 480}};
  quit_waiter quit_waiter_;
};

int main(int argc, char** argv) try {
  log4cplus::Initializer log_init;
  if (const auto log_cfg = xdg::find_config("wayland-test/log.cfg"); !log_cfg.empty())
    log4cplus::PropertyConfigurator{log_cfg}.configure();
  else
    log4cplus::BasicConfigurator{}.configure();

  wl::display display{argc > 1 ? argv[1] : nullptr};

  registry_searcher<wl::compositor, wl::shell, wl::shm, wl::seat> searcher;

  wl::registry registry = display.get_registry();
  registry.add_listener(searcher);
  wl::callback sync_cb = display.sync();
  sync_cb.add_listener(searcher);

  ui_application app;
  pc::future<int> f = searcher.on_found([&app](auto... a) {return app.start(std::move(a)...);});
  while (!f.is_ready())
    display.dispatch();
  return f.get();
} catch (const std::exception& err) {
  std::cerr << "Error: " << err.what() << '\n';
  return  EXIT_FAILURE;
}
