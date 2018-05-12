#include <array>
#include <iostream>

#include <portable_concurrency/future>

#include "wayland/client.hpp"

#include "io.h"
#include "registry_searcher.hpp"
#include "xdg.h"

using namespace std::literals;

class shared_memory {
public:
  shared_memory(size_t size):
    fd_{io::open(xdg::runtime_dir(), io::mode::read_write | io::mode::tmpfile | io::mode::close_exec)}
  {
    io::truncate(fd_, size);
    mem_ = io::mmap(size, io::prot::read | io::prot::write, io::map::shared, fd_);
  }

  std::byte* data() noexcept {return mem_.data();}
  const std::byte* data() const noexcept {return mem_.data();}
  size_t size() const noexcept {return mem_.size();}

  const io::file_descriptor& fd() const noexcept {return fd_;}

private:
  io::file_descriptor fd_;
  io::mem_mapping mem_;
};

pc::future<int> start(wl::display& display, wl::compositor compositor, wl::shell shell, wl::shm shm) {
  std::cout << "Compositor version: " << compositor.get_version() << '\n';
  std::cout << "Shell version: " << shell.get_version() << '\n';
  std::cout << "SHM version: " << shm.get_version() << '\n';

  wl::surface surface = compositor.create_surface();
  std::cout << "Created a surface of version: " << surface.get_version() << '\n';
  wl::shell_surface shsurf = shell.get_shell_surface(surface);
  std::cout << "Created a shell surface of version: " << shsurf.get_version() << '\n';
  shsurf.set_toplevel();

  wl::shm::listener shm_listener = [](wl::shm::ref, wl::shm::format fmt) {
    std::cout << "\tsupported pixel format code: " << std::hex << static_cast<uint32_t>(fmt) << '\n';
  };
  shm.add_listener(shm_listener);

  struct shell_surface_logger {
    void ping(wl::shell_surface::ref surf, uint32_t serial) {
      std::cout << "ping: " << serial << '\n';
      surf.pong(serial);
    }
    void configure(wl::shell_surface::ref, uint32_t edges, uint32_t w, uint32_t h) {
      std::cout << "\tshell_surface cofiguration: edges = " << edges << "size = " << w << 'x' << h << '\n';
    }
    void popup_done(wl::shell_surface::ref) {}
  };
  wl::shell_surface::listener<shell_surface_logger> sh_srf_listener;
  shsurf.add_listener(sh_srf_listener);

  shared_memory shmem{1024};
  wl::shm::pool pool = shm.create_pool(shmem.fd().native_handle(), shmem.size());
  std::cout << "wl_shm_pool version: " << pool.get_version() << '\n';

  pc::promise<void> p;
  wl::callback::listener sync_listener = [&](wl::callback::ref, uint32_t) {p.set_value();};
  wl::callback sync_cb = display.sync();
  sync_cb.add_listener(sync_listener);
  co_await p.get_future();

  co_return EXIT_SUCCESS;
}

int main(int argc, char** argv) try {
  wl::display display{argc > 1 ? argv[1] : nullptr};

  registry_searcher<wl::compositor, wl::shell, wl::shm> searcher;

  wl::registry::listener iface_listener = std::ref(searcher);
  wl::registry registry = display.get_registry();
  registry.add_listener(iface_listener);

  wl::callback::listener sync_listener = std::ref(searcher);
  wl::callback sync_cb = display.sync();
  sync_cb.add_listener(sync_listener);

  using namespace std::placeholders;
  auto f = searcher.on_found(std::bind(start, std::ref(display), _1, _2, _3));
  while (!f.is_ready())
    display.dispatch();
  return f.get();
} catch (const std::exception& err) {
  std::cerr << "Error: " << err.what() << '\n';
  return  EXIT_FAILURE;
}
