#include <array>
#include <iostream>

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/configurator.h>
#include <log4cplus/initializer.h>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <portable_concurrency/future>

#include <cairomm/context.h>
#include <cairomm/fontface.h>
#include <cairomm/surface.h>

#include "io/file.hpp"
#include "io/shmem.hpp"

#include "wayland/client.hpp"

#include "registry_searcher.hpp"
#include "quit_waiter.hpp"

using namespace std::literals;
using namespace fmt::literals;

namespace {

size_t buffer_size(Cairo::Format fmt, wl::size sz) {
  return static_cast<size_t>(Cairo::ImageSurface::format_stride_for_width(fmt, sz.width)*sz.width*sz.height);
}

}

class window {
public:
  window(wl::size size):
    size_{size},
    buffers_memory_{buffer_size(Cairo::FORMAT_ARGB32, size)}
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
    buffer_ = pool.create_buffer(
      0, size_, Cairo::ImageSurface::format_stride_for_width(Cairo::FORMAT_ARGB32, size_.width),
      wl::shm::format::ARGB8888
    );
    LOG4CPLUS_DEBUG(log_, "Created a wl::buffer of version: {}"_format(buffer_.get_version()));
    draw_content();
    surface_.attach(buffer_);
    surface_.commit();
  }

  void draw_content() {
    const int stride = Cairo::ImageSurface::format_stride_for_width(Cairo::FORMAT_ARGB32, size_.width);
    auto img_surf = Cairo::ImageSurface::create(
      reinterpret_cast<unsigned char*>(buffers_memory_.data()), Cairo::FORMAT_ARGB32, size_.width, size_.height, stride
    );
    auto ctx = Cairo::Context::create(img_surf);
    ctx->set_source_rgb(0.8, 0.8, 0.8);
    ctx->rectangle(0, 0, size_.width, size_.height);
    ctx->fill();

    ctx->move_to(10, 30);
    auto default_font = Cairo::ToyFontFace::create("", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
    LOG4CPLUS_DEBUG(log_, "Default font family: {}"_format(default_font->get_family()));
    ctx->set_font_face(default_font);
    ctx->set_font_size(14);
    ctx->set_source_rgb(0.0, 0.0, 0.0);
    ctx->show_text("Press ESC to quit");
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
