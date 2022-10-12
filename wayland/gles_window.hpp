#include <wayland/egl.hpp>
#include <wayland/gles2/renderer.hpp>
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
