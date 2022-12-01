#include <functional>
#include <optional>
#include <variant>

#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>
#include <asio/static_thread_pool.hpp>

#include <wayland/egl.hpp>
#include <wayland/ivi_window.hpp>
#include <wayland/util/channel.hpp>
#include <wayland/util/geom.hpp>
#include <wayland/vsync_frames.hpp>
#include <wayland/xdg_window.hpp>

class gles_context {
public:
  gles_context(wl_display &display, wl_surface &surf, size sz);
  ~gles_context() noexcept;

  gles_context(const gles_context &) = delete;
  gles_context &operator=(const gles_context &) = delete;
  gles_context(gles_context &&) = delete;
  gles_context &operator=(gles_context &&) = delete;

  [[nodiscard]] size get_size() const noexcept;
  bool resize(size sz);
  egl::surface &egl_surface() noexcept { return egl_surface_; }

private:
  egl::surface egl_surface_;
  wl::unique_ptr<wl_egl_window> egl_wnd_;
};

class gles_window {
public:
  using any_window = std::variant<xdg::toplevel_window, ivi::window>;
  using render_function =
      std::function<void(gles_context &glctx, vsync_frames &frames,
                         value_update_channel<size> &resize_channel)>;

  gles_window() noexcept = default;
  gles_window(event_loop &eloop, asio::thread_pool::executor_type pool_exec,
              any_window wnd, size initial_size, render_function render_func);

  gles_window(const gles_window &) = delete;
  gles_window &operator=(const gles_window &) = delete;

  gles_window(gles_window &&) noexcept;
  gles_window &operator=(gles_window &&) noexcept;

  ~gles_window() noexcept;

  static asio::awaitable<gles_window>
  create_maximized(event_loop &eloop, asio::io_context::executor_type io_exec,
                   asio::thread_pool::executor_type pool_exec,
                   render_function render_func);

  [[nodiscard]] bool is_closed() const noexcept;

private:
  struct impl;

private:
  any_window wnd_;
  std::unique_ptr<impl> impl_;
};
