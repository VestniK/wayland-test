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

struct kb_handler {
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

  log4cplus::Logger log;
  pc::promise<void> promise;
  xkb::keymap kmap;
  wl::keycode esc_keycode = {};
};

pc::future<void> wait_quit(wl::seat& seat) {
  std::cout << "==== Press Esc to quit ====\n";
  log4cplus::Logger log = log4cplus::Logger::getInstance("INPUT");

  LOG4CPLUS_DEBUG(log, "Seat version: {}"_format(seat.get_version()));
  struct {
    void capabilities(wl::seat::ref, ut::bitmask<wl::seat::capability> caps) {
      LOG4CPLUS_DEBUG(log, "Seat capabilities: {}"_format(caps.value()));
    }
    void name(wl::seat::ref, const char* seat_name) {
      LOG4CPLUS_DEBUG(log, "Seat name: {}"_format(seat_name));
    }

    log4cplus::Logger log;
  } seat_logger{log};
  seat.add_listener(seat_logger);

  kb_handler kb_listener{seat_logger.log};
  wl::keyboard kb = seat.get_keyboard();
  kb.add_listener(kb_listener);

  co_await kb_listener.promise.get_future();
}

pc::future<int> start(wl::display& display, wl::compositor compositor, wl::shell shell, wl::shm shm, wl::seat seat) {
  auto log = log4cplus::Logger::getInstance("UI");

  LOG4CPLUS_DEBUG(log, "Compositor version: {}"_format(compositor.get_version()));
  LOG4CPLUS_DEBUG(log, "Shell version: {}"_format(shell.get_version()));
  LOG4CPLUS_DEBUG(log, "SHM version: {}"_format(shm.get_version()));

  wl::surface surface = compositor.create_surface();
  LOG4CPLUS_DEBUG(log, "Created a surface of version: {}"_format(surface.get_version()));
  wl::shell_surface shsurf = shell.get_shell_surface(surface);
  LOG4CPLUS_DEBUG(log, "Created a shell surface of version: {}"_format(shsurf.get_version()));
  shsurf.set_toplevel();

  auto shm_listener = [log](wl::shm::ref, wl::shm::format fmt) {
    LOG4CPLUS_DEBUG(log, "supported pixel format code: {:x}"_format(ut::underlying_cast(fmt)));
  };
  shm.add_listener(shm_listener);

  struct {
    void ping(wl::shell_surface::ref surf, wl::serial serial) {
      LOG4CPLUS_DEBUG(log, "ping: {}"_format(serial));
      surf.pong(serial);
    }
    void configure(wl::shell_surface::ref, uint32_t edges, wl::size sz) {
      LOG4CPLUS_DEBUG(log, "shell_surface cofiguration: edges: {}; size: {}x{}"_format(edges, sz.width, sz.height));
    }
    void popup_done(wl::shell_surface::ref) {
      LOG4CPLUS_DEBUG(log, "popup done");
    }

    log4cplus::Logger log;
  } shell_surface_logger{log};
  shsurf.add_listener(shell_surface_logger);

  io::shared_memory shmem{4*240*480};
  wl::shm::pool pool = shm.create_pool(shmem.fd().native_handle(), shmem.size());
  LOG4CPLUS_DEBUG(log, "Created a wl::shm::pool of version: {}"_format(pool.get_version()));
  wl::buffer buf = pool.create_buffer(0, wl::size{240, 480}, 4*240, wl::shm::format::ARGB8888);
  LOG4CPLUS_DEBUG(log, "Created a wl::buffer of version: {}"_format(buf.get_version()));
  std::fill(shmem.data(), shmem.data() + shmem.size(), std::byte{0x88});
  surface.attach(buf);
  surface.commit();

  pc::promise<void> p;
  auto sync_listener = [&](wl::callback::ref, uint32_t) {p.set_value();};
  wl::callback sync_cb = display.sync();
  sync_cb.add_listener(sync_listener);
  auto [sync_res, quit_res] = co_await pc::when_all(p.get_future(), wait_quit(seat));
  sync_res.get();
  quit_res.get();

  co_return EXIT_SUCCESS;
}

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

  using namespace std::placeholders;
  pc::future<int> f = searcher.on_found(std::bind(start, std::ref(display), _1, _2, _3, _4));
  while (!f.is_ready())
    display.dispatch();
  return f.get();
} catch (const std::exception& err) {
  std::cerr << "Error: " << err.what() << '\n';
  return  EXIT_FAILURE;
}
