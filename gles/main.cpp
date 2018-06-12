#include <chrono>
#include <iostream>

#include <wayland/client.hpp>
#include <wayland/egl.hpp>

#include <EGL/egl.h>

#include <GL/gl.h>
//#include <GLES2/gl2.h>

#if defined(minor)
#undef minor
#endif

#if defined(major)
#undef major
#endif

using namespace std::literals;

template<typename Service>
struct idetified {
  Service service;
  wl::id id = {};
};

struct watched_services {
  idetified<wl::compositor> compositor;
  idetified<wl::shell> shell;
  bool initial_sync_done = false;

  void check() {
    if (!compositor.service)
      throw std::runtime_error{"Compositor is not available"};
    if (!shell.service)
      throw std::runtime_error{"Shell is not available"};
  }

  void global(wl::registry::ref reg, wl::id id, std::string_view name, wl::version ver) {
    if (name == wl::compositor::interface_name)
      compositor = {reg.bind<wl::compositor>(id, ver), id};
    if (name == wl::shell::interface_name)
      shell = {reg.bind<wl::shell>(id, ver), id};
  }
  void global_remove(wl::registry::ref, wl::id id) {
    if (id == compositor.id)
      compositor = {{}, {}};
    if (id == shell.id)
      shell = {{}, {}};
  }

  void operator() (wl::callback::ref, uint32_t) {
    initial_sync_done = true;
  }
};

namespace egl {

const std::error_category& category() {
  static const struct : std::error_category {
    const char* name() const noexcept override {return "EGL";}
    std::string message(int cond) const override {
      switch (cond) {
      case EGL_SUCCESS:
        return "Success";
      case EGL_NOT_INITIALIZED:
        return "EGL is not initialized, or could not be initialized, for the specified EGL display connection.";
      case EGL_BAD_ACCESS:
        return "EGL cannot access a requested resource (for example a context is bound in another thread).";
      case EGL_BAD_ALLOC:
        return "EGL failed to allocate resources for the requested operation.";
      case EGL_BAD_ATTRIBUTE:
        return "An unrecognized attribute or attribute value was passed in the attribute list.";
      case EGL_BAD_CONTEXT:
        return "An EGLContext argument does not name a valid EGL rendering context.";
      case EGL_BAD_CONFIG:
        return "An EGLConfig argument does not name a valid EGL frame buffer configuration.";
      case EGL_BAD_CURRENT_SURFACE:
        return "The current surface of the calling thread is a window, pixel buffer or pixmap that is no longer valid.";
      case EGL_BAD_DISPLAY:
        return "An EGLDisplay argument does not name a valid EGL display connection.";
      case EGL_BAD_SURFACE:
        return "An EGLSurface argument does not name a valid surface (window, pixel buffer or pixmap) configured for "
               "GL rendering.";
      case EGL_BAD_MATCH:
        return "Arguments are inconsistent (for example, a valid context requires buffers not supplied by a valid "
               "surface).";
      case EGL_BAD_PARAMETER:
        return "One or more argument values are invalid.";
      case EGL_BAD_NATIVE_PIXMAP:
        return "A NativePixmapType argument does not refer to a valid native pixmap.";
      case EGL_BAD_NATIVE_WINDOW:
        return "A NativeWindowType argument does not refer to a valid native window.";
      case EGL_CONTEXT_LOST:
        return "A power management event has occurred. The application must destroy all contexts and reinitialise "
               "OpenGL ES state and objects to continue rendering.";
      }
      return "Unknown error " + std::to_string(cond);
    }
  } instance;
  return instance;
}

class surface {
public:
  surface(EGLDisplay disp, EGLConfig cfg, const wl::egl::window& wnd): disp_(disp) {
    surf_ = eglCreateWindowSurface(disp_, cfg, wnd.native_handle(), nullptr);
    if (surf_ == EGL_NO_SURFACE)
      throw std::system_error{eglGetError(), category(), "eglCreaeglCreateWindowSurfaceteContext"};
  }
  ~surface() {
    eglDestroySurface(disp_, surf_);
  }

  surface(const surface&) = delete;
  surface& operator= (const surface&) = delete;
  surface(surface&&) = delete;
  surface& operator= (surface&&) = delete;

  using native_handle_type = EGLSurface;
  native_handle_type native_handle() const noexcept {return surf_;}

  void swap_buffers() {
    if (eglSwapBuffers(disp_, surf_) == EGL_FALSE)
      throw std::system_error{eglGetError(), category(), "eglSwapBuffers"};
  }

private:
  EGLDisplay disp_;
  EGLSurface surf_;
};

class context {
public:
  explicit context(EGLDisplay disp, EGLConfig cfg): disp_(disp), cfg_(cfg) {
    const EGLint attr[] = {
      EGL_CONTEXT_CLIENT_VERSION, 2,
      EGL_NONE
    };
    ctx_ = eglCreateContext(disp_, cfg_, EGL_NO_CONTEXT, attr);
    if (ctx_ == EGL_NO_CONTEXT)
      throw std::system_error{eglGetError(), category(), "eglCreateContext"};
  }
  ~context() {
    eglDestroyContext(disp_, ctx_);
  }

  context(const context&) = delete;
  context& operator= (const context&) = delete;
  context(context&&) = delete;
  context& operator= (context&&) = delete;

  using native_handle_type = EGLContext;
  native_handle_type native_handle() const noexcept {return ctx_;}

  EGLConfig get_config() const noexcept {return cfg_;}

  void make_current(const surface& surf) {
    if (eglMakeCurrent(disp_, surf.native_handle(), surf.native_handle(), ctx_) == EGL_FALSE)
      throw std::system_error{eglGetError(), category(), "eglMakeCurrent"};
  }

private:
  EGLDisplay disp_;
  EGLConfig cfg_;
  EGLContext ctx_;
};

class display {
public:
  display() noexcept = default;

  explicit display(const wl::display& native_display): disp_(eglGetDisplay(native_display.native_handle())) {
    if (disp_ == EGL_NO_DISPLAY)
      throw std::runtime_error{"Failed to get EGL display"};
    if (eglInitialize(disp_, &major_, &minor_) == EGL_FALSE)
      throw std::system_error{eglGetError(), category(), "eglInitialize"};
  }

  display(const display&) = delete;
  display& operator= (const display&) = delete;
  display(display&&) = delete;
  display& operator= (display&&) = delete;

  ~display() noexcept {
    if (disp_ != EGL_NO_DISPLAY)
      eglTerminate(disp_);
  }

  using native_handle_type = EGLDisplay;
  native_handle_type native_handle() const noexcept {return disp_;}

  explicit operator bool () const noexcept {return disp_ != EGL_NO_DISPLAY;}

  EGLint major() const noexcept {return major_;}
  EGLint minor() const noexcept {return minor_;}

  void bind_api(EGLenum api) {
    if (eglBindAPI(api) != EGL_TRUE)
      throw std::system_error{eglGetError(), category(), "eglBindAPI"};
  }

  context create_context(EGLConfig cfg) {
    return context{disp_, cfg};
  }

  surface create_surface(EGLConfig cfg, const wl::egl::window& wnd) {
    return surface{disp_, cfg, wnd};
  }

  EGLConfig choose_config() {
    EGLConfig res;
    EGLint count;
    const EGLint attr[] = {
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
      EGL_RED_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_BLUE_SIZE, 8,
      EGL_ALPHA_SIZE, 8,
      EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
      EGL_NONE
    };
    if (eglChooseConfig(disp_, attr, &res, 1, &count) == EGL_FALSE)
      throw std::system_error{eglGetError(), category(), "eglChooseConfig"};
    return res;
  }

private:
  EGLDisplay disp_ = EGL_NO_DISPLAY;
  EGLint major_;
  EGLint minor_;
};

}

struct window_listener {
  void ping(wl::shell_surface::ref surf, wl::serial serial) {
    surf.pong(serial);
  }
  void configure(wl::shell_surface::ref, uint32_t, wl::size) {}
  void popup_done(wl::shell_surface::ref) {}
};

int main(int argc, char** argv) try {
  wl::display display(argc > 1 ? argv[1] : nullptr);

  watched_services services;
  wl::registry reg = display.get_registry();
  reg.add_listener(services);
  wl::callback sync_cb = display.sync();
  sync_cb.add_listener(services);

  while (
    !services.initial_sync_done ||
    !services.compositor.service ||
    !services.shell.service
  )
    display.dispatch();
  services.check();

  wl::surface surface = services.compositor.service.create_surface();
  wl::shell_surface shell_surface = services.shell.service.get_shell_surface(surface);
  shell_surface.set_toplevel();
  window_listener pong_responder;
  shell_surface.add_listener(pong_responder);

  egl::display edisp{display};
  egl::context ctx = edisp.create_context(edisp.choose_config());
  edisp.bind_api(EGL_OPENGL_API);

  wl::egl::window wnd(surface, {640, 480});
  egl::surface esurf = edisp.create_surface(ctx.get_config(), wnd);
  ctx.make_current(esurf);

  glClearColor(0.0, 0.0, 0.0, 0.9);
  glShadeModel(GL_FLAT);

  glViewport(0, 0, 640, 480);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  auto ts = std::chrono::steady_clock::now();
  while (true) {
    const std::chrono::steady_clock::time_point last = std::exchange(ts, std::chrono::steady_clock::now());
    const GLfloat color = std::chrono::duration_cast<std::chrono::milliseconds>((ts - last)%5s).count()/5000.;
    glClear(GL_COLOR_BUFFER_BIT);
    glColor4f(color, color, 1.0, 1.0);
    glBegin(GL_POLYGON);
      glVertex3f(0.25, 0.25, 0.0);
      glVertex3f(0.75, 0.25, 0.0);
      glVertex3f(0.75, 0.75, 0.0);
      glVertex3f(0.25, 0.75, 0.0);
    glEnd();
    glFlush();
    esurf.swap_buffers();
    display.dispatch_pending();
  }

  return EXIT_SUCCESS;
} catch(const std::exception& error) {
  std::cerr << error.what() << '\n';
  return EXIT_FAILURE;
}
