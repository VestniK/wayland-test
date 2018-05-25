#include <exception>
#include <iostream>

#include <gsl/span>

#include <wayland/client.hpp>
#include <io/shmem.hpp>

template<typename Service>
struct idetified {
  Service service;
  wl::id id = {};
};

struct watched_services {
  idetified<wl::compositor> compositor;
  idetified<wl::shell> shell;
  idetified<wl::shm> shm;
  bool initial_sync_done = false;

  void check() {
    if (!compositor.service)
      throw std::runtime_error{"Compositor is not available"};
    if (!shell.service)
      throw std::runtime_error{"Shell is not available"};
    if (!shm.service)
      throw std::runtime_error{"SHM is not available"};
  }

  void global(wl::registry::ref reg, wl::id id, std::string_view name, wl::version ver) {
    if (name == wl::compositor::interface_name)
      compositor = {reg.bind<wl::compositor>(id, ver), id};
    if (name == wl::shell::interface_name)
      shell = {reg.bind<wl::shell>(id, ver), id};
    if (name == wl::shm::interface_name)
      shm = {reg.bind<wl::shm>(id, ver), id};
  }
  void global_remove(wl::registry::ref, wl::id id) {
    if (id == compositor.id)
      compositor = {{}, {}};
    if (id == shell.id)
      shell = {{}, {}};
    if (id == shm.id)
      shm = {{}, {}};
  }

  void operator() (wl::callback::ref, uint32_t) {
    initial_sync_done = true;
  }
};

class buffer {
  friend class window;
public:
  buffer(wl::shm::ref shm, wl::size size):
    storage_{buffer_mem_size(size)}
  {
    wl::shm::pool pool = shm.create_pool(storage_.fd().native_handle(), storage_.size());
    buffer_ = pool.create_buffer(0, size, buffer_stride(size), wl::shm::format::XRGB8888);
  }

  wl::buffer::ref get_buffer() {
    return buffer_;
  }

  gsl::span<std::byte> get_memory() {
    return {storage_.data(), static_cast<gsl::span<std::byte>::size_type>(storage_.size())};
  }

private:
  static size_t buffer_mem_size(wl::size size) {return static_cast<size_t>(buffer_stride(size)*size.height);}
  static size_t buffer_stride(wl::size size) {return static_cast<size_t>(4*size.width);}

private:
  io::shared_memory storage_;
  wl::buffer buffer_;
};

class window {
public:
  window(wl::compositor::ref compositor, wl::shell::ref shell):
    surf_(compositor.create_surface()),
    sh_surf_(shell.get_shell_surface(surf_))
  {
    sh_surf_.add_listener(*this);
    sh_surf_.set_toplevel();
    sh_surf_.set_title("Animation wayland example");
  }

  void set_buffer(wl::buffer::ref buf) {
    surf_.attach(buf);
  }

  wl::callback redraw(int left, int top, wl::size sz) {
    auto res = surf_.frame();
    surf_.damage(left, top, sz);
    surf_.commit();
    return res;
  }

  void ping(wl::shell_surface::ref surf, wl::serial serial) {
    surf.pong(serial);
  }
  void configure(wl::shell_surface::ref, uint32_t, wl::size) {}
  void popup_done(wl::shell_surface::ref) {}

private:
  wl::surface surf_;
  wl::shell_surface sh_surf_;
};

int main(int argc, char** argv) try {
  wl::display display{argc > 1 ? argv[1] : nullptr};

  watched_services services;
  wl::registry reg = display.get_registry();
  reg.add_listener(services);
  wl::callback sync_cb = display.sync();
  sync_cb.add_listener(services);

  while (
    !services.initial_sync_done ||
    !services.compositor.service ||
    !services.shell.service ||
    !services.shm.service
  )
    display.dispatch();
  services.check();

  const wl::size wnd_sz = {640, 480};
  window wnd(services.compositor.service, services.shell.service);
  buffer buf{services.shm.service, wnd_sz};
  wnd.set_buffer(buf.get_buffer());

  bool can_redraw = true;
  auto on_can_redraw = [&can_redraw](wl::callback::ref, uint32_t) {can_redraw = true;};
  uint8_t intense = 0;
  wl::callback frame_cb;
  while (true) {
    display.dispatch();
    services.check();
    if (std::exchange(can_redraw, false)) {
      gsl::span<std::byte> mem = buf.get_memory();
      std::fill(mem.begin(), mem.end(), static_cast<std::byte>(intense++));
      frame_cb = wnd.redraw(0, 0, wnd_sz);
      frame_cb.add_listener(on_can_redraw);
    }
  }

  return EXIT_SUCCESS;
} catch (const std::exception& error) {
  std::cerr << error.what() << '\n';
  return EXIT_FAILURE;
}
