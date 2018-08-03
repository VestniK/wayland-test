#include <chrono>
#include <iostream>
#include <optional>
#include <vector>

#include "client.hpp"

#include <EGL/egl.h>

#include <GLES2/gl2.h>

#include <gsl/string_span>
#include <gsl/pointers>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#if defined(minor)
#undef minor
#endif

#if defined(major)
#undef major
#endif

using namespace std::literals;

template<typename Service>
struct identified {
  wl::unique_ptr<Service> service;
  uint32_t id = {};
};

struct watched_services {
  wl_registry_listener listener = {&global, &global_remove};
  identified<wl_compositor> compositor;
  identified<wl_shell> shell;
  std::vector<identified<wl_output>> outputs;
  bool initial_sync_done = false;

  void check() {
    if (!compositor.service)
      throw std::runtime_error{"Compositor is not available"};
    if (!shell.service)
      throw std::runtime_error{"Shell is not available"};
    if (outputs.empty())
      throw std::runtime_error{"No outputs available"};
  }

  static void global(void* data, wl_registry* reg, uint32_t id, const char* name, uint32_t ver) {
    watched_services* self = reinterpret_cast<watched_services*>(data);
    if (name == wl::service_trait<wl_compositor>::name)
      self->compositor = {wl::bind<wl_compositor>(reg, id, ver), id};
    if (name == wl::service_trait<wl_shell>::name)
      self->shell = {wl::bind<wl_shell>(reg, id, ver), id};
    if (name == wl::service_trait<wl_output>::name)
      self->outputs.push_back({wl::bind<wl_output>(reg, id, ver), id});
  }
  static void global_remove(void* data, wl_registry*, uint32_t id) {
    watched_services* self = reinterpret_cast<watched_services*>(data);
    if (id == self->compositor.id)
      self->compositor = {{}, {}};
    if (id == self->shell.id)
      self->shell = {{}, {}};
    if (
      const auto it = std::find_if(
        self->outputs.begin(), self->outputs.end(), [&](const auto& item) {return item.id == id;}
      );
      it != self->outputs.end()
    )
      self->outputs.erase(it);
  }
};

class window {
public:
  struct delegate {
    /// @pre wnd.valid()
    virtual void resize(window& wnd, uint32_t edges, wl::size sz) = 0;
  };

public:
  window() noexcept = default;

  window(const window&) = delete;
  window& operator= (const window&) = delete;

  window(window&& rhs) noexcept:
    surface_{std::move(rhs.surface_)},
    shell_surface_{std::move(rhs.shell_surface_)}
  {
    if (shell_surface_)
      wl_shell_surface_set_user_data(shell_surface_.get(), this);
  }

  window& operator= (window&& rhs) noexcept {
    surface_ = std::move(rhs.surface_);
    shell_surface_ = std::move(rhs.shell_surface_);
    if (shell_surface_)
      wl_shell_surface_set_user_data(shell_surface_.get(), this);
    return *this;
  }

  ~window() = default;

  static window create_toplevel(wl_compositor& compositor, wl_shell& shell) {
    wl::unique_ptr<wl_surface> surface{wl_compositor_create_surface(&compositor)};
    wl::unique_ptr<wl_shell_surface> shell_surf{wl_shell_get_shell_surface(&shell, surface.get())};
    wl_shell_surface_set_toplevel(shell_surf.get());
    return {std::move(surface), std::move(shell_surf)};
  }

  void set_delegate(delegate* val) {delegate_ = val;}

  bool valid() const noexcept {return surface_ && shell_surface_;}

  wl_surface* get_surface() {return surface_.get();}

  /// @pre this->valid()
  void maximize(wl_output* output = nullptr) {
    wl_shell_surface_set_maximized(shell_surface_.get(), output);
  }

private:
  window(wl::unique_ptr<wl_surface> surface, wl::unique_ptr<wl_shell_surface> shell_surface) noexcept:
    surface_{std::move(surface)},
    shell_surface_{std::move(shell_surface)}
  {
    wl_shell_surface_add_listener(shell_surface_.get(), &shell_surface_listener, this);
  }

  static void ping(void*, wl_shell_surface* surf, uint32_t serial) {wl_shell_surface_pong(surf, serial);}
  static void configure(void* data, wl_shell_surface*, uint32_t edges, int32_t w, int32_t h) {
    auto* self = reinterpret_cast<window*>(data);
    if (self->delegate_)
      self->delegate_->resize(*self, edges, {w, h});
  }
  static void popup_done(void*, wl_shell_surface*) {}

  static wl_shell_surface_listener shell_surface_listener;

private:
  wl::unique_ptr<wl_surface> surface_;
  wl::unique_ptr<wl_shell_surface> shell_surface_;
  delegate* delegate_ = nullptr;
};

wl_shell_surface_listener window::shell_surface_listener = {
  .ping = window::ping,
  .configure = window::configure,
  .popup_done = window::popup_done
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
  surface() = default;

  surface(EGLDisplay disp, EGLConfig cfg, gsl::not_null<wl_egl_window*> wnd) {
    reset(disp, cfg, wnd);
  }
  ~surface() {
    if (surf_ != EGL_NO_SURFACE)
      eglDestroySurface(disp_, surf_);
  }

  void reset(EGLDisplay disp, EGLConfig cfg, gsl::not_null<wl_egl_window*> wnd) {
    if (surf_ != EGL_NO_SURFACE)
      eglDestroySurface(disp_, surf_);
    surf_ = eglCreateWindowSurface(disp, cfg, wnd, nullptr);
    if (surf_ == EGL_NO_SURFACE)
      throw std::system_error{eglGetError(), category(), "eglCreaeglCreateWindowSurfaceteContext"};
    disp_ = disp;
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
  EGLDisplay disp_ = EGL_NO_DISPLAY;
  EGLSurface surf_ = EGL_NO_SURFACE;
};

class context {
public:
  explicit context(EGLDisplay disp, EGLConfig cfg): disp_(disp), cfg_(cfg) {
    const EGLint attr[] = {
      EGL_CONTEXT_MAJOR_VERSION, 2,
      EGL_CONTEXT_MINOR_VERSION, 0,
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
  explicit display(gsl::not_null<wl_display*> native_display): disp_(eglGetDisplay(native_display)) {
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

  surface create_surface(EGLConfig cfg, gsl::not_null<wl_egl_window*> wnd) {
    return surface{disp_, cfg, wnd};
  }

  void reset_surface(surface& surf, EGLConfig cfg, gsl::not_null<wl_egl_window*> wnd) {
    surf.reset(disp_, cfg, wnd);
  }

  EGLConfig choose_config() {
    bind_api(EGL_OPENGL_ES_API);
    EGLConfig res;
    EGLint count;
    const EGLint attr[] = {
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
      EGL_RED_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_BLUE_SIZE, 8,
      EGL_ALPHA_SIZE, 8,
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
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

class shader {
public:
  using native_handle_type = GLuint;
  enum class type: GLenum {
    vertex = GL_VERTEX_SHADER,
    fragment = GL_FRAGMENT_SHADER
  };

  shader() noexcept = default;

  shader(type type, gsl::czstring<> src) {
    shader_ = glCreateShader(static_cast<GLenum>(type));
    if(shader_ == 0)
      throw std::runtime_error{"Failed to create shader"};
    glShaderSource(shader_, 1, &src, nullptr);
    glCompileShader(shader_);
    GLint compiled;
    glGetShaderiv(shader_, GL_COMPILE_STATUS, &compiled);
    if(!compiled) {
      GLint msg_size;
      glGetShaderiv(shader_, GL_INFO_LOG_LENGTH, &msg_size);
      std::string msg;
      if(msg_size > 1) {
        msg.resize(msg_size);
        glGetShaderInfoLog(shader_, msg_size, nullptr, msg.data());
      } else
        msg = "unknown compiation error";
      glDeleteShader(shader_);
      throw std::runtime_error{"Failed to compile shader: " + msg};
    }
  }

  ~shader() {
    if (shader_ != 0)
      glDeleteShader(shader_);
  }

  shader(const shader&) = delete;
  shader& operator= (const shader&) = delete;

  shader(shader&& rhs) noexcept: shader_(std::exchange(rhs.shader_, 0)) {}
  shader& operator= (shader&& rhs) noexcept {
    if (shader_ != 0)
      glDeleteShader(shader_);
    shader_ = std::exchange(rhs.shader_, 0);
    return *this;
  }

  native_handle_type native_handle() const noexcept {return shader_;}

  explicit operator bool () const noexcept {return shader_ != 0;}

private:
  GLuint shader_ = 0;
};

class elements_array_buffer {
public:
  using native_handle_type = GLuint;
  native_handle_type native_handle() const {return handle_;}

  elements_array_buffer() {
    glGenBuffers(1, &handle_);
  }

  ~elements_array_buffer() {
    glDeleteBuffers(1, &handle_);
  }

private:
  GLuint handle_ = 0;
};

class renderer {
  using clock = std::chrono::steady_clock;
public:
  renderer():
    vertex_shader_(shader::type::vertex, R"(
      uniform mat4 rotation;
      attribute vec4 position;
      void main() {
        gl_Position = rotation * position;
      }
    )"),
    fragment_shader_(shader::type::fragment, R"(
      precision mediump float;
      uniform vec4 hotspot;
      void main() {
        float norm_dist = length(gl_FragCoord - hotspot)/90.0;
        float factor = exp(-norm_dist*norm_dist);
        vec4 color = vec4(0.0, 0.0, 1.0, 1.0);
        gl_FragColor = vec4(color.r*factor, color.g*factor, color.b*factor, 1);
      }
    )")
  {
    program_ = glCreateProgram();
    if (program_ == 0)
      throw std::runtime_error{"Failed to create GLSL program"};
    glAttachShader(program_, vertex_shader_.native_handle());
    glAttachShader(program_, fragment_shader_.native_handle());
    glBindAttribLocation(program_, 0, "vPosition");
    glLinkProgram(program_);

    GLint linked;
    glGetProgramiv(program_, GL_LINK_STATUS, &linked);
    if(!linked) {
       GLint msg_size = 0;
       glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &msg_size);
       std::string msg;
       if(msg_size > 1) {
         msg.resize(msg_size);
         glGetProgramInfoLog(program_, msg_size, nullptr, msg.data());
       } else
         msg = "unknown link error";
       glDeleteProgram(program_);
       throw std::runtime_error{"Faild to link GLSL program: " + msg};
    }

    static const GLuint idxs[] = {0, 1, 2, 1, 3, 2};
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idxs_.native_handle());
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idxs), idxs, GL_STATIC_DRAW);
  }
  ~renderer() {
    glDeleteProgram(program_);
  }

  void resize(wl::size sz) {
    glClearColor(0, 0, 0, .9);
    len_ = std::min(sz.width, sz.height);
    glViewport(0, 0, len_, len_);
  }

  void draw(clock::time_point ts) {
    struct vertex {
      GLfloat x = 0;
      GLfloat y = 0;
    };
    static const vertex vertices[] = {
      {-.5, .5},
      {.5, .5},
      {-.5, -.5},
      {.5, -.5}
    };

    constexpr auto period = 3s;
    constexpr auto spot_period = 7s;

    const GLfloat phase = (ts.time_since_epoch()%period).count()/GLfloat(clock::duration{period}.count());
    const GLfloat spot_phase = (ts.time_since_epoch()%spot_period).count()/GLfloat(clock::duration{spot_period}.count());

    const GLfloat spot_angle = 2*M_PI*spot_phase;
    const GLfloat angle = 2*M_PI*phase;
    glm::vec4 hotspot = {len_/2 + 7*len_/24*std::cos(spot_angle), 5*len_/12 + len_/24*std::cos(2*spot_angle), 0, 0};
    glm::mat4 rotation = glm::rotate(glm::mat4{1.}, angle, {0, 0, 1});

    GLint rotation_uniform = glGetUniformLocation(program_, "rotation");
    GLint hotspot_uniform = glGetUniformLocation(program_, "hotspot");
    GLint position_location = glGetAttribLocation(program_, "position");

    glClear(GL_COLOR_BUFFER_BIT);

    glUniformMatrix4fv(rotation_uniform, 1, GL_FALSE, glm::value_ptr(rotation));
    glUniform4fv(hotspot_uniform, 1, glm::value_ptr(hotspot));

    glUseProgram(program_);
    glVertexAttribPointer(position_location, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(position_location);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idxs_.native_handle());
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glFlush();
  }

private:
  shader vertex_shader_;
  shader fragment_shader_;
  GLuint program_ = 0;
  elements_array_buffer idxs_;
  int32_t len_;
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
    wl::size sz;
    std::optional<renderer> drawer;

    virtual void resize(window& wnd, uint32_t, wl::size new_sz) override {
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
  } delegate{display.get()};

  auto wnd = window::create_toplevel(*services.compositor.service, *services.shell.service);
  wnd.set_delegate(&delegate);
  wnd.maximize(services.outputs.front().service.get());
  delegate.resize(wnd, 0, {480, 480});

  while (true) {
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
