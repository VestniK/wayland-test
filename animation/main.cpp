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

class double_buffer {
public:
  double_buffer(wl::shm::ref shm, wl::size size):
    size_{size},
    storage_{2*buffer_mem_size()}
  {
    wl::shm::pool pool = shm.create_pool(storage_.fd().native_handle(), storage_.size());
    buffers_[0] = pool.create_buffer(0, size_, buffer_stride(), wl::shm::format::XRGB8888);
    buffers_[1] = pool.create_buffer(buffer_mem_size(), size_, buffer_stride(), wl::shm::format::XRGB8888);
  }

  wl::buffer::ref swap() {
    const wl::buffer::ref res = buffers_[active_];
    active_ = (active_ + 1)%buffers_.size();
    return res;
  }

  gsl::span<std::byte> active_buffer() {
    return {
      storage_.data() + active_*buffer_mem_size(),
      static_cast<gsl::span<std::byte>::size_type>(buffer_mem_size())
    };
  }

private:
  size_t buffer_mem_size() {return static_cast<size_t>(buffer_stride()*size_.height);}
  size_t buffer_stride() {return static_cast<size_t>(4*size_.width);}

private:
  wl::size size_;
  io::shared_memory storage_;
  std::array<wl::buffer, 2> buffers_;
  size_t active_ = 0;
};

class window {
public:
  window(wl::compositor::ref compositor, wl::shell::ref shell):
    surf_(compositor.create_surface()),
    sh_surf_(shell.get_shell_surface(surf_))
  {
    sh_surf_.add_listener(*this);
    sh_surf_.set_toplevel();
  }

  void draw(wl::buffer::ref buf) {
    surf_.attach(buf);
    surf_.commit();
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

  window wnd(services.compositor.service, services.shell.service);
  double_buffer buffrs{services.shm.service, {640, 480}};
  {
    gsl::span<std::byte> buf = buffrs.active_buffer();
    std::fill(buf.begin(), buf.end(), std::byte{0xc0});
  }
  wnd.draw(buffrs.swap());

  while (true)
    display.dispatch(), services.check();

  return EXIT_SUCCESS;
} catch (const std::exception& error) {
  std::cerr << error.what() << '\n';
  return EXIT_FAILURE;
}
