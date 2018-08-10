#include <chrono>
#include <iostream>
#include <optional>
#include <vector>

#include <gsl/string_span>
#include <gsl/pointers>

#include "client.hpp"
#include "egl.h"
#include "renderer.h"

using namespace std::literals;

template<typename Service>
struct identified {
  wl::unique_ptr<Service> service;
  uint32_t id = {};
};

struct watched_services {
  wl_registry_listener listener = {&global, &global_remove};
  identified<wl_compositor> compositor;
  identified<zxdg_shell_v6> shell;
  bool initial_sync_done = false;

  void check() {
    if (!compositor.service)
      throw std::runtime_error{"Compositor is not available"};
    if (!shell.service)
      throw std::runtime_error{"Shell is not available"};
  }

  static void global(void* data, wl_registry* reg, uint32_t id, const char* name, uint32_t ver) {
    watched_services* self = reinterpret_cast<watched_services*>(data);
    if (name == wl::service_trait<wl_compositor>::name)
      self->compositor = {wl::bind<wl_compositor>(reg, id, ver), id};
    if (name == wl::service_trait<zxdg_shell_v6>::name)
      self->shell = {wl::bind<zxdg_shell_v6>(reg, id, ver), id};
  }
  static void global_remove(void* data, wl_registry*, uint32_t id) {
    watched_services* self = reinterpret_cast<watched_services*>(data);
    if (id == self->compositor.id)
      self->compositor = {{}, {}};
    if (id == self->shell.id)
      self->shell = {{}, {}};
  }
};

class window {
public:
  struct delegate {
    /// @pre wnd.valid()
    virtual void resize(window& wnd, size sz) = 0;
    virtual void close(window& wnd) = 0;
  };

public:
  window() noexcept = default;

  window(const window&) = delete;
  window& operator= (const window&) = delete;

  window(window&& rhs) noexcept:
    surface_{std::move(rhs.surface_)},
    toplevel_{std::move(rhs.toplevel_)}
  {
    if (toplevel_)
      zxdg_toplevel_v6_set_user_data(toplevel_.get(), this);
  }

  window& operator= (window&& rhs) noexcept {
    surface_ = std::move(rhs.surface_);
    toplevel_ = std::move(rhs.toplevel_);
    if (toplevel_)
      zxdg_toplevel_v6_set_user_data(toplevel_.get(), this);
    return *this;
  }

  ~window() = default;

  static window create_toplevel(wl_compositor& compositor, zxdg_shell_v6& shell) {
    wl::unique_ptr<wl_surface> surface{wl_compositor_create_surface(&compositor)};
    wl::unique_ptr<zxdg_surface_v6> xdg_surf{zxdg_shell_v6_get_xdg_surface(&shell, surface.get())};
    wl::unique_ptr<zxdg_toplevel_v6> toplevel{zxdg_surface_v6_get_toplevel(xdg_surf.get())};
    return {std::move(surface), std::move(xdg_surf), std::move(toplevel)};
  }

  void set_delegate(delegate* val) {delegate_ = val;}

  bool valid() const noexcept {return surface_ && toplevel_;}

  wl_surface* get_surface() {return surface_.get();}

  /// @pre this->valid()
  void maximize() {
    zxdg_toplevel_v6_set_maximized(toplevel_.get());
  }

private:
  window(
    wl::unique_ptr<wl_surface> surface,
    wl::unique_ptr<zxdg_surface_v6> xdg_surf,
    wl::unique_ptr<zxdg_toplevel_v6> toplevel
  ) noexcept:
    surface_{std::move(surface)},
    xdg_surface_{std::move(xdg_surf)},
    toplevel_{std::move(toplevel)}
  {
    zxdg_toplevel_v6_add_listener(toplevel_.get(), &toplevel_listener, this);
    zxdg_surface_v6_add_listener(xdg_surface_.get(), &xdg_surface_listener, this);
  }

  static void configure_surface(void*, zxdg_surface_v6*, uint32_t) {}
  static void configure_toplevel(void* data, zxdg_toplevel_v6*, int32_t width, int32_t height, wl_array*) {
    auto* self = reinterpret_cast<window*>(data);
    if (self->delegate_)
      self->delegate_->resize(*self, {width, height});
  }
  static void close(void* data, zxdg_toplevel_v6*) {
    auto* self = reinterpret_cast<window*>(data);
    if (self->delegate_)
      self->delegate_->close(*self);
  }

  static zxdg_toplevel_v6_listener toplevel_listener;
  static zxdg_surface_v6_listener xdg_surface_listener;

private:
  wl::unique_ptr<wl_surface> surface_;
  wl::unique_ptr<zxdg_surface_v6> xdg_surface_;
  wl::unique_ptr<zxdg_toplevel_v6> toplevel_;
  delegate* delegate_ = nullptr;
};

zxdg_toplevel_v6_listener window::toplevel_listener = {
  .configure = window::configure_toplevel,
  .close = window::close
};

zxdg_surface_v6_listener window::xdg_surface_listener = {
  .configure = window::configure_surface
};

int main(int argc, char** argv) try {
  wl::unique_ptr<wl_display> display{wl_display_connect(argc > 1 ? argv[1] : nullptr)};
  if (!display)
    throw std::system_error{errno, std::system_category(), "wl_display_connect"};

  watched_services services;
  wl::unique_ptr<wl_registry> reg{wl_display_get_registry(display.get())};
  wl_registry_add_listener(reg.get(), &services.listener, &services);
  wl_display_roundtrip(display.get());
  services.check();

  struct egl_delegate : public window::delegate {
    explicit egl_delegate(gsl::not_null<wl_display*> display): egl_display(display) {}

    egl::display egl_display;
    EGLConfig cfg = egl_display.choose_config();
    egl::context ctx = egl_display.create_context(cfg);
    wl::unique_ptr<wl_egl_window> egl_wnd;
    egl::surface egl_surface;
    size sz;
    std::optional<renderer> drawer;
    bool closed = false;

    void resize(window& wnd, size new_sz) override {
      if (sz == new_sz)
        return;
      sz = new_sz;
      egl_wnd = wl::unique_ptr<wl_egl_window>{wl_egl_window_create(wnd.get_surface(), sz.width, sz.height)};
      egl_display.reset_surface(egl_surface, cfg, egl_wnd.get());
      ctx.make_current(egl_surface);
      if (!drawer)
        drawer.emplace();
      drawer->resize(sz);
    }

    void close(window&) override {closed = true;}
  } delegate{display.get()};

  auto wnd = window::create_toplevel(*services.compositor.service, *services.shell.service);
  wnd.set_delegate(&delegate);
  wnd.maximize();

  while (!delegate.closed) {
    if (delegate.drawer) {
      delegate.drawer->draw(std::chrono::steady_clock::now());
      delegate.egl_surface.swap_buffers();
      wl_display_dispatch_pending(display.get());
    } else
      wl_display_dispatch(display.get());
  }

  return EXIT_SUCCESS;
} catch(const std::exception& error) {
  std::cerr << error.what() << '\n';
  return EXIT_FAILURE;
}
