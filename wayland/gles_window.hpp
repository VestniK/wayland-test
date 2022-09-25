#include <filesystem>
#include <memory>
#include <optional>

#include <wayland/egl.hpp>
#include <wayland/event_loop.hpp>
#include <wayland/gles2/renderer.hpp>
#include <wayland/script_player.hpp>
#include <wayland/util/geom.hpp>
#include <wayland/xdg_window.hpp>

class gles_delegate final : public xdg::delegate, public script_window {
public:
  gles_delegate(event_loop &eloop, wl_surface &surf, size sz);
  ~gles_delegate() noexcept override;

  bool paint();

  [[nodiscard]] size get_size() const noexcept;

  // script_window
  std::error_code draw_frame() override;
  std::error_code draw_for(std::chrono::milliseconds duration) override;
  void camera_look_at(float ex, float ey, float ez, float cx, float cy,
                      float cz) override;

  // toplevel_window
  void resize(size sz) override;
  void close() override {}

private:
  event_loop &eloop_;
  egl::surface egl_surface_;
  wl::unique_ptr<wl_egl_window> egl_wnd_;
  std::optional<renderer> renderer_;
};
