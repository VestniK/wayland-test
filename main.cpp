#include <array>
#include <iostream>

#include <portable_concurrency/future>

#include "wayland/client.hpp"

#include "io.h"
#include "registry_searcher.hpp"
#include "xdg.h"
#include "xkb.hpp"

using namespace std::literals;

class shared_memory {
public:
  shared_memory(io::file_descriptor fd, size_t size): fd_(std::move(fd)) {
    io::truncate(fd_, size);
    mem_ = io::mmap(size, io::prot::read | io::prot::write, io::map::shared, fd_);
  }

  explicit shared_memory(size_t size):
    shared_memory(io::open(xdg::runtime_dir(), io::mode::read_write | io::mode::tmpfile | io::mode::close_exec), size)
  {}

  std::byte* data() noexcept {return mem_.data();}
  const std::byte* data() const noexcept {return mem_.data();}
  size_t size() const noexcept {return mem_.size();}

  const io::file_descriptor& fd() const noexcept {return fd_;}

private:
  io::file_descriptor fd_;
  io::mem_mapping mem_;
};

pc::future<void> wait_quit(wl::seat& seat) {
  std::cout << "==== Press Esc to quit ====\n";
  struct seat_logger {
    void capabilities(wl::seat::ref, wl::bitmask<wl::seat::capability> caps) {
      std::cout << "\tSeat capabilities: " << caps.value() << '\n';
    }
    void name(wl::seat::ref, const char* seat_name) {std::cout << "\tSeat name: " << seat_name << '\n';}
  };
  wl::seat::listener<seat_logger> listner;
  seat.add_listener(listner);

  pc::promise<void> p;
  pc::future esc_future = p.get_future();
  struct kb_logger {
    kb_logger(pc::promise<void> p): promise{std::move(p)} {}

    void keymap(wl::keyboard::ref, wl::keyboard::keymap_format fmt, int fd, size_t size) try {
      shared_memory shmem{io::file_descriptor{fd}, size};
      std::cout << "Received keymap. Format: " << wl::underlying_cast(fmt) << "; of size: " << size << '\n';
      if (fmt != wl::keyboard::keymap_format::xkb_v1) {
        promise.set_exception(std::make_exception_ptr(std::runtime_error{"Unsupported keymap format"}));
        return;
      }
      xkb::context ctx;
      xkb::keymap kmap = ctx.load_keyap(shmem.data(), shmem.size() - 1);
      const xkb_keycode_t esc_xkb = kmap.key_by_name("ESC");
      if (esc_xkb == XKB_KEYCODE_INVALID)
        throw std::runtime_error{"failed to find Escape key code"};
      esc_keycode = esc_xkb - 8; // TODO: wrap conversion properly with strong typedef
      std::cout << "Escape keycode: " << esc_keycode << '\n';
    } catch(...) {
      promise.set_exception(std::current_exception());
    }

    void enter(wl::keyboard::ref, wl::serial eid, wl::surface::ref, wl_array*) {
      esc_pressed = false;
      std::cout << "Surface get focus[" << eid << "]\n";
    }
    void leave(wl::keyboard::ref, wl::serial eid, wl::surface::ref) {
      esc_pressed = false;
      std::cout << "Surface lost focus[" << eid << "]\n";
    }
    void key(wl::keyboard::ref, wl::serial eid, wl::clock::time_point time, uint32_t key, wl::keyboard::key_state state) {
      std::cout
        << "Key event[" << eid << "] timestamp: " << time.time_since_epoch().count() << "; key code: " << key
        << "; key state: " << wl::underlying_cast(state) << '\n';

      constexpr uint32_t esc_keycde = 1;
      if (key != esc_keycde) {
        esc_pressed = false;
        return;
      }

      if (esc_pressed && state == wl::keyboard::key_state::released) {
        promise.set_value();
        return;
      }

      esc_pressed = state == wl::keyboard::key_state::pressed;
    }
    void modifiers(
      wl::keyboard::ref, wl::serial eid, uint32_t mods_depressed, uint32_t mods_latched,
      uint32_t mods_locked, uint32_t group
    ) {
      esc_pressed = false;
      std::cout
        << "Modifiers event[" << eid << "] mds_depresed: " << mods_depressed << "; mods latched: " << mods_latched
        << "; mods locked: " << mods_locked << "; group: " << group << '\n';
    }
    void repeat_info(wl::keyboard::ref, int32_t rate, std::chrono::milliseconds delay) {
      std::cout << "Repeat info: rate: " << rate << "; delay: " << delay.count() << "ms\n";
    }

    pc::promise<void> promise;
    uint32_t esc_keycode = 0;
    bool esc_pressed = false;
  };
  wl::keyboard::listener<kb_logger> kb_listener{std::move(p)};
  wl::keyboard kb = seat.get_keyboard();
  kb.add_listener(kb_listener);

  co_await esc_future;
}

pc::future<int> start(wl::display& display, wl::compositor compositor, wl::shell shell, wl::shm shm, wl::seat seat) {
  std::cout << "Compositor version: " << compositor.get_version() << '\n';
  std::cout << "Shell version: " << shell.get_version() << '\n';
  std::cout << "SHM version: " << shm.get_version() << '\n';
  std::cout << "Seat version: " << seat.get_version() << '\n';

  wl::surface surface = compositor.create_surface();
  std::cout << "Created a surface of version: " << surface.get_version() << '\n';
  wl::shell_surface shsurf = shell.get_shell_surface(surface);
  std::cout << "Created a shell surface of version: " << shsurf.get_version() << '\n';
  shsurf.set_toplevel();

  wl::shm::listener shm_listener = [](wl::shm::ref, wl::shm::format fmt) {
    const auto flags = std::cout.flags();
    std::cout << "\tsupported pixel format code: " << std::hex << static_cast<uint32_t>(fmt) << '\n';
    std::cout.setf(flags);
  };
  shm.add_listener(shm_listener);

  struct shell_surface_logger {
    void ping(wl::shell_surface::ref surf, wl::serial serial) {
      std::cout << "ping: " << serial << '\n';
      surf.pong(serial);
    }
    void configure(wl::shell_surface::ref, uint32_t edges, wl::size sz) {
      std::cout << "\tshell_surface cofiguration: edges = " << edges << "size = " << sz.width << 'x' << sz.height << '\n';
    }
    void popup_done(wl::shell_surface::ref) {}
  };
  wl::shell_surface::listener<shell_surface_logger> sh_srf_listener;
  shsurf.add_listener(sh_srf_listener);

  shared_memory shmem{4*240*480};
  wl::shm::pool pool = shm.create_pool(shmem.fd().native_handle(), shmem.size());
  std::cout << "wl_shm_pool version: " << pool.get_version() << '\n';
  wl::buffer buf = pool.create_buffer(0, wl::size{240, 480}, 4*240, wl::shm::format::ARGB8888);
  std::cout << "wl_buffer version: " << buf.get_version() << '\n';
  std::fill(shmem.data(), shmem.data() + shmem.size(), std::byte{0x88});
  surface.attach(buf);
  surface.commit();

  pc::promise<void> p;
  wl::callback::listener sync_listener = [&](wl::callback::ref, uint32_t) {p.set_value();};
  wl::callback sync_cb = display.sync();
  sync_cb.add_listener(sync_listener);
  auto [sync_res, quit_res] = co_await pc::when_all(p.get_future(), wait_quit(seat));
  sync_res.get();
  quit_res.get();

  co_return EXIT_SUCCESS;
}

int main(int argc, char** argv) try {
  wl::display display{argc > 1 ? argv[1] : nullptr};

  registry_searcher<wl::compositor, wl::shell, wl::shm, wl::seat> searcher;

  wl::registry::listener iface_listener = std::ref(searcher);
  wl::registry registry = display.get_registry();
  registry.add_listener(iface_listener);

  wl::callback::listener sync_listener = std::ref(searcher);
  wl::callback sync_cb = display.sync();
  sync_cb.add_listener(sync_listener);

  using namespace std::placeholders;
  auto f = searcher.on_found(std::bind(start, std::ref(display), _1, _2, _3, _4));
  while (!f.is_ready())
    display.dispatch();
  return f.get();
} catch (const std::exception& err) {
  std::cerr << "Error: " << err.what() << '\n';
  return  EXIT_FAILURE;
}
