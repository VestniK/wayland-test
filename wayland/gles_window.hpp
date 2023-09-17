#include <concepts>
#include <functional>
#include <optional>
#include <variant>

#include <asio/static_thread_pool.hpp>

#include <util/channel.hpp>
#include <util/geom.hpp>

#include <wayland/egl.hpp>
#include <wayland/shell_window.hpp>
#include <wayland/vsync_frames.hpp>

namespace wl {
class gui_shell;
}

class gles_context {
public:
  gles_context(wl_display& display, wl_surface& surf, size sz);
  ~gles_context() noexcept;

  gles_context(const gles_context&) = delete;
  gles_context& operator=(const gles_context&) = delete;
  gles_context(gles_context&&) = delete;
  gles_context& operator=(gles_context&&) = delete;

  [[nodiscard]] size get_size() const noexcept;
  bool resize(size sz);
  egl::surface& egl_surface() noexcept { return egl_surface_; }

private:
  egl::surface egl_surface_;
  wl::unique_ptr<wl_egl_window> egl_wnd_;
};

class gles_window {
public:
  using render_function = std::function<void(gles_context& glctx,
      vsync_frames& frames, value_update_channel<size>& resize_channel)>;

  gles_window() noexcept = default;
  gles_window(event_loop& eloop, asio::thread_pool::executor_type pool_exec,
      wl::sized_window<wl::shell_window>&& wnd, render_function render_func);

  gles_window(const gles_window&) = delete;
  gles_window& operator=(const gles_window&) = delete;

  gles_window(gles_window&&) noexcept;
  gles_window& operator=(gles_window&&) noexcept;

  ~gles_window() noexcept;

  [[nodiscard]] bool is_closed() const noexcept;

private:
  struct impl;

private:
  wl::shell_window wnd_;
  std::unique_ptr<impl> impl_;
};

template <typename T>
concept renderer = requires(T& t, size sz, frames_clock::time_point tp) {
  { t.resize(sz) };
  { t.draw(tp) };
};

template <renderer Renderer, typename... A>
  requires std::constructible_from<Renderer, A...>
gles_window::render_function make_render_func(A&&... a) {
  return
      [... args = std::forward<A>(a)](gles_context& ctx, vsync_frames& frames,
          value_update_channel<size>& resize_channel) mutable {
        Renderer render{std::move(args)...};
        render.resize(ctx.get_size());

        for (auto frame_time : frames) {
          if (const auto sz = resize_channel.get_update()) {
            ctx.resize(sz.value());
            render.resize(sz.value());
          }

          render.draw(frame_time);
          ctx.egl_surface().swap_buffers();
        }
      };
}
