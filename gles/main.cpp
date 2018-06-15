#include <chrono>
#include <iostream>

#include <wayland/client.hpp>
#include <wayland/egl.hpp>

#include <EGL/egl.h>

#include <GLES2/gl2.h>

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

class triangle_drawer {
public:
  triangle_drawer():
    vertex_shader_(shader::type::vertex, R"(
      uniform mat4 rotation;
      attribute vec4 position;
      void main() {
        gl_Position = rotation * position;
      }
    )"),
    fragment_shader_(shader::type::fragment, R"(
      precision mediump float;
      uniform vec4 color;
      void main() {
        gl_FragColor = color;
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
  }
  ~triangle_drawer() {
    glDeleteProgram(program_);
  }

  void resize(wl::size sz) {
    glClearColor(0., .3, 0., 0.85);
    glViewport(0, 0, sz.width, sz.height);
  }

  void draw(wl::clock::time_point ts) {
    struct vertex {
      GLfloat x = 0;
      GLfloat y = 0;
    };
    static const vertex vertices[] = {
      {0., .5},
      {-.5, -.5},
      {.5, -.5}
    };
    constexpr auto period = 3s;

    const GLfloat phase = (ts.time_since_epoch()%period).count()/GLfloat(wl::clock::duration{period}.count());
    const GLfloat angle = 2*M_PI*phase;
    const GLfloat color_component = std::abs(2*phase - 1.);
    glm::vec4 color = {color_component, color_component, 1., 1.};
    glm::mat4 rotation = glm::rotate(glm::mat4{1.}, angle, {0, 0, 1});

    GLint rotation_uniform = glGetUniformLocation(program_, "rotation");
    GLint color_uniform = glGetUniformLocation(program_, "color");
    GLint position_location = glGetAttribLocation(program_, "position");

    glClear(GL_COLOR_BUFFER_BIT);

    glUniformMatrix4fv(rotation_uniform, 1, GL_FALSE, glm::value_ptr(rotation));
    glUniform4fv(color_uniform, 1, glm::value_ptr(color));

    glUseProgram(program_);
    glVertexAttribPointer(position_location, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(position_location);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glFlush();
  }

private:
  shader vertex_shader_;
  shader fragment_shader_;
  GLuint program_ = 0;
};

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
  edisp.bind_api(EGL_OPENGL_ES_API);
  EGLConfig cfg = edisp.choose_config();
  egl::context ctx = edisp.create_context(cfg);

  const wl::size wnd_sz = {480, 480};
  wl::egl::window wnd(surface, wnd_sz);
  egl::surface esurf = edisp.create_surface(cfg, wnd);
  ctx.make_current(esurf);

  triangle_drawer drawer;
  drawer.resize(wnd_sz);

  while (true) {
    drawer.draw(wl::clock::time_point{
      std::chrono::duration_cast<wl::clock::duration>(std::chrono::steady_clock::now().time_since_epoch())
    });
    esurf.swap_buffers();
    display.dispatch_pending();
  }

  return EXIT_SUCCESS;
} catch(const std::exception& error) {
  std::cerr << error.what() << '\n';
  return EXIT_FAILURE;
}
