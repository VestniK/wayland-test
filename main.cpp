#include <chrono>
#include <optional>

#include "client.hpp"
#include "egl.h"
#include "renderer.h"
#include "window.h"

using namespace std::literals;

template<typename Service>
struct identified {
  wl::unique_ptr<Service> service;
  uint32_t id = {};
};

struct watched_services {
  wl_registry_listener listener = {&global, &global_remove};
  identified<wl_compositor> compositor;
  identified<xdg_wm_base> shell;

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
    if (name == wl::service_trait<xdg_wm_base>::name)
      self->shell = {wl::bind<xdg_wm_base>(reg, id, ver), id};
  }
  static void global_remove(void* data, wl_registry*, uint32_t id) {
    watched_services* self = reinterpret_cast<watched_services*>(data);
    if (id == self->compositor.id)
      self->compositor = {{}, {}};
    if (id == self->shell.id)
      self->shell = {{}, {}};
  }
};

egl::context make_egl_context(wl_display& display) {
  egl::display egl_display{display};
  egl_display.bind_api(EGL_OPENGL_ES_API);
  EGLint count;
  const EGLint cfg_attr[] = {
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
  };
  EGLConfig cfg;
  if (eglChooseConfig(egl_display.native_handle(), cfg_attr, &cfg, 1, &count) == EGL_FALSE)
    throw std::system_error{eglGetError(), egl::category(), "eglChooseConfig"};

  return egl::context{std::move(egl_display), cfg};
}

class egl_window: public toplevel_window {
public:
  egl_window(wl_display& display, wl_compositor& compositor, xdg_wm_base& wm):
    toplevel_window(compositor, wm),
    egl_surface_(make_egl_context(display))
  {}

  ~egl_window() noexcept override {
    if (egl_surface_)
      egl_surface_.release_thread();
  }

  bool is_closed() const noexcept {return  closed;}
  bool paint() {
    if (!drawer_)
      return false;

    drawer_->draw(std::chrono::steady_clock::now());
    egl_surface_.swap_buffers();
    return true;
  }

  size get_size() const noexcept {
    size sz;
    wl_egl_window_get_attached_size(egl_wnd_.get(), &sz.width, &sz.height);
    return sz;
  }

protected:
  void resize(size sz) override {
    if (egl_wnd_ && get_size() == sz)
      return;
    if (!egl_wnd_) {
      egl_wnd_ = wl::unique_ptr<wl_egl_window>{wl_egl_window_create(get_surface(), sz.width, sz.height)};
      egl_surface_.set_window(*egl_wnd_);
    } else
      wl_egl_window_resize(egl_wnd_.get(), sz.width, sz.height, 0, 0);
    egl_surface_.make_current();
    if (!drawer_)
      drawer_.emplace();
    drawer_->resize(sz);
  }

  void close() override {closed = true;}

private:
  egl::surface egl_surface_;
  wl::unique_ptr<wl_egl_window> egl_wnd_;
  std::optional<renderer> drawer_;
  bool closed = false;
};

int main(int argc, char** argv) {
  wl::unique_ptr<wl_display> display{wl_display_connect(argc > 1 ? argv[1] : nullptr)};
  if (!display)
    throw std::system_error{errno, std::system_category(), "wl_display_connect"};

  watched_services services;
  wl::unique_ptr<wl_registry> reg{wl_display_get_registry(display.get())};
  wl_registry_add_listener(reg.get(), &services.listener, &services);
  wl_display_roundtrip(display.get());
  services.check();

  egl_window wnd{*display, *services.compositor.service, *services.shell.service};
  wnd.maximize();

  while (!wnd.is_closed()) {
    services.check();
    if (wnd.paint())
      wl_display_dispatch_pending(display.get());
    else
      wl_display_dispatch(display.get());
  }

  return EXIT_SUCCESS;
}
