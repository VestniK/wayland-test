#include <filesystem>
#include <memory>
#include <optional>

#include <wayland/egl.hpp>
#include <wayland/event_loop.hpp>
#include <wayland/gles2/renderer.hpp>
#include <wayland/util/geom.hpp>
#include <wayland/xdg_window.hpp>

class gles_delegate final : public xdg::delegate {
public:
  gles_delegate(const event_loop &eloop, wl_surface &surf, size sz);
  ~gles_delegate() noexcept override;

  bool paint();

  [[nodiscard]] size get_size() const noexcept;
  [[nodiscard]] bool is_closed() const noexcept { return closed_; }

  // toplevel_window
  void resize(size sz) override;
  void close() override { closed_ = true; }

private:
  egl::surface egl_surface_;
  wl::unique_ptr<wl_egl_window> egl_wnd_;
  std::optional<renderer> renderer_;
  bool closed_ = false;
};
