#include <atomic>
#include <stop_token>
#include <variant>

#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>
#include <asio/static_thread_pool.hpp>

#include <wayland/egl.hpp>
#include <wayland/gles2/renderer.hpp>
#include <wayland/ivi_window.hpp>
#include <wayland/util/geom.hpp>
#include <wayland/xdg_window.hpp>

class event_loop;

class gles_context {
public:
  gles_context(const event_loop &eloop, wl_surface &surf, size sz);
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

class gles_delegate final : public xdg::delegate {
public:
  gles_delegate(const event_loop &eloop, wl_surface &surf, size sz);

  bool paint();

  [[nodiscard]] bool is_closed() const noexcept { return closed_; }

  // toplevel_window
  void resize(size sz) override;
  void close() override { closed_ = true; }

private:
  gles_context ctx_;
  renderer renderer_;
  bool closed_ = false;
};

class gles_window {
public:
  using any_window = std::variant<xdg::toplevel_window, ivi::window>;
  gles_window() noexcept = default;
  gles_window(event_loop &eloop, asio::thread_pool::executor_type pool_exec,
              any_window wnd, size initial_size);

  gles_window(const gles_window &) = delete;
  gles_window &operator=(const gles_window &) = delete;

  gles_window(gles_window &&) noexcept = default;
  gles_window &operator=(gles_window &&) noexcept = default;

  ~gles_window() noexcept;

  static asio::awaitable<gles_window>
  create(event_loop &eloop, asio::io_context::executor_type io_exec,
         asio::thread_pool::executor_type pool_exec);

  [[nodiscard]] bool is_closed() const noexcept {
    return !closed_ || closed_->test();
  }

private:
  std::stop_source stop_;
  std::unique_ptr<std::atomic_flag> closed_;
};
