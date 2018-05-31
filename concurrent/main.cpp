#include <exception>
#include <iostream>
#include <optional>

#include <gsl/span>

#include <cairomm/context.h>
#include <cairomm/surface.h>

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
  idetified<wl::seat> seat;
  idetified<wl::subcompositor> subcompositor;
  bool initial_sync_done = false;

  void check() {
    if (!compositor.service)
      throw std::runtime_error{"Compositor is not available"};
    if (!shell.service)
      throw std::runtime_error{"Shell is not available"};
    if (!shm.service)
      throw std::runtime_error{"SHM is not available"};
    if (!seat.service)
      throw std::runtime_error{"Seat is not available"};
    if (!subcompositor.service)
      throw std::runtime_error{"Subcompositor is not available"};
  }

  void global(wl::registry::ref reg, wl::id id, std::string_view name, wl::version ver) {
    if (name == wl::compositor::interface_name)
      compositor = {reg.bind<wl::compositor>(id, ver), id};
    if (name == wl::shell::interface_name)
      shell = {reg.bind<wl::shell>(id, ver), id};
    if (name == wl::shm::interface_name)
      shm = {reg.bind<wl::shm>(id, ver), id};
    if (name == wl::subcompositor::interface_name)
      subcompositor = {reg.bind<wl::subcompositor>(id, ver), id};
    if (!seat.service && name == wl::seat::interface_name)
      seat = {reg.bind<wl::seat>(id, ver), id};
  }
  void global_remove(wl::registry::ref, wl::id id) {
    if (id == compositor.id)
      compositor = {{}, {}};
    if (id == shell.id)
      shell = {{}, {}};
    if (id == shm.id)
      shm = {{}, {}};
    if (id == subcompositor.id)
      subcompositor = {{}, {}};
    if (id == seat.id)
      seat = {{}, {}};
  }

  void operator() (wl::callback::ref, uint32_t) {
    initial_sync_done = true;
  }
};

class buffer {
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

struct window_listener {
  void ping(wl::shell_surface::ref surf, wl::serial serial) {
    surf.pong(serial);
  }
  void configure(wl::shell_surface::ref, uint32_t, wl::size) {}
  void popup_done(wl::shell_surface::ref) {}
};

struct ball_in_box {
  using clock = wl::clock;

  wl::size box_size = {640, 480};
  wl::point start_pos = {box_size.width/4, box_size.height/2};
  int radius = 12;
  int velocity_x = 153;
  int velocity_y = 138;
  clock::time_point start_time = clock::time_point::max();

  wl::point calculate_movement(clock::time_point time) {
    if (start_time > time) {
      start_time = time;
      return start_pos;
    }
    const auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(time - start_time);

    wl::point pos = {
      start_pos.x + static_cast<int>(velocity_x*interval.count()/1000),
      start_pos.y + static_cast<int>(velocity_y*interval.count()/1000)
    };

    const wl::point min = {radius, radius};
    const wl::point max = {box_size.width - radius, box_size.height - radius};

    const bool has_reflections = pos.x < min.x || pos.x > max.x || pos.y < min.y || pos.y > max.y;

    while (pos.x < min.x || pos.x > max.x) {
      if (pos.x > max.x)
        pos.x = 2*max.x - pos.x;
      if (pos.x < min.x)
        pos.x = 2*min.x - pos.x;
      velocity_x = -velocity_x;
    }

    while (pos.y < min.y || pos.y > max.y) {
      if (pos.y > max.y)
        pos.y = 2*max.y - pos.y;
      if (pos.y < min.y)
        pos.y = 2*min.y - pos.y;
      velocity_y = -velocity_y;
    }

    if (has_reflections)
    {
      start_pos = pos;
      start_time = time;
    }
    return pos;
  }

  void draw(gsl::span<std::byte> frame_buf, clock::time_point now) {
    const wl::point frame_pos = calculate_movement(now);

    const int stride = Cairo::ImageSurface::format_stride_for_width(Cairo::FORMAT_ARGB32, box_size.width);
    auto img_surf = Cairo::ImageSurface::create(
      reinterpret_cast<unsigned char*>(frame_buf.data()), Cairo::FORMAT_ARGB32, box_size.width, box_size.height, stride
    );
    auto ctx = Cairo::Context::create(img_surf);
    ctx->set_source_rgb(0.8, 0.8, 0.8);
    ctx->rectangle(0, 0, box_size.width, box_size.height);
    ctx->fill();

    ctx->set_source_rgb(0.2, 0.8, 0.2);
    ctx->arc(frame_pos.x, frame_pos.y, radius, 0, 2*M_PI);
    ctx->fill();
  }
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
    !services.shm.service ||
    !services.seat.service ||
    !services.subcompositor.service
  )
    display.dispatch();
  services.check();

  const int decor_height = 35;
  wl::surface wnd_surface = services.compositor.service.create_surface();
  wl::surface content_surface = services.compositor.service.create_surface();
  wl::subsurface content = services.subcompositor.service.get_subsurface(content_surface, wnd_surface);
  content.set_desync();
  content.set_position(wl::point{0, decor_height});

  window_listener pong_responder;
  wl::shell_surface shell_surf = services.shell.service.get_shell_surface(wnd_surface);
  shell_surf.add_listener(pong_responder);
  shell_surf.set_toplevel();

  ball_in_box scene;
  buffer wnd_buf(services.shm.service, wl::size{scene.box_size.width, decor_height});
  buffer content_buf(services.shm.service,scene.box_size);

  scene.draw(content_buf.get_memory(), wl::clock::time_point::max());
  content_surface.attach(content_buf.get_buffer());
  content_surface.commit();

  gsl::span<std::byte> mem = wnd_buf.get_memory();
  std::fill(mem.begin(), mem.end(), std::byte{0x70});
  wnd_surface.attach(wnd_buf.get_buffer());
  wnd_surface.commit();

  bool need_redraw = true;
  auto on_can_redraw = [&](wl::callback::ref, uint32_t ts) {
    scene.draw(content_buf.get_memory(), wl::clock::time_point{wl::clock::duration{ts}});
    need_redraw = true;
  };
  wl::callback frame_cb;
  while (true) {
    display.dispatch();
    services.check();
    if (need_redraw) {
      frame_cb = content_surface.frame();
      content_surface.damage({{}, scene.box_size});
      content_surface.commit();
      frame_cb.add_listener(on_can_redraw);
    }
  }

  return EXIT_SUCCESS;
} catch (const std::exception& error) {
  std::cerr << error.what() << '\n';
  return EXIT_FAILURE;
}
