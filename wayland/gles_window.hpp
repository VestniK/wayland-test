#include <functional>
#include <optional>
#include <stop_token>
#include <variant>

#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>
#include <asio/static_thread_pool.hpp>

#include <wayland/egl.hpp>
#include <wayland/ivi_window.hpp>
#include <wayland/util/channel.hpp>
#include <wayland/util/clock.hpp>
#include <wayland/util/geom.hpp>
#include <wayland/xdg_window.hpp>

class event_queue;
class event_loop;

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

class vsync_frames {
public:
  struct iterator;
  struct sentinel {};
  using value_type = frames_clock::time_point;

  vsync_frames(event_queue &queue, wl_surface &surf, std::stop_token &stop);

  iterator begin();
  sentinel end() const { return {}; }

private:
  std::optional<value_type> wait();

private:
  event_queue &queue_;
  wl_surface &surf_;
  wl::unique_ptr<wl_callback> frame_cb_;
  std::stop_token &stop_;
};

struct vsync_frames::iterator {
  using value_type = vsync_frames::value_type;
  using iterator_category = std::input_iterator_tag;
  using reference = const value_type &;
  using pointer = const value_type *;
  using difference_type = std::ptrdiff_t;

  reference operator*() const noexcept { return current; }
  iterator &operator++() {
    if (auto next = vsync->wait())
      current = next.value();
    else
      vsync = nullptr;

    return *this;
  }
  iterator operator++(int) {
    auto res = *this;
    ++(*this);
    return res;
  }

  vsync_frames *vsync = nullptr;
  value_type current;
};

inline vsync_frames::iterator vsync_frames::begin() {
  if (auto first = wait())
    return iterator{.vsync = this, .current = first.value()};
  else
    return iterator{.vsync = nullptr, .current = {}};
}

inline constexpr bool operator==(const vsync_frames::iterator &it,
                                 vsync_frames::sentinel) noexcept {
  return it.vsync == nullptr;
}

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
