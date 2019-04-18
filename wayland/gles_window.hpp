#include <filesystem>
#include <memory>
#include <optional>

#include <wayland/egl.hpp>
#include <wayland/event_loop.hpp>
#include <wayland/gles2/renderer.hpp>
#include <wayland/script_player.hpp>
#include <wayland/util/geom.hpp>
#include <wayland/window.hpp>

class gles_window final : public toplevel_window, public script_window {
public:
  gles_window(event_loop& eloop);
  ~gles_window() noexcept override;

  bool is_closed() const noexcept { return closed; }
  bool is_initialized() const noexcept;
  bool paint();

  size get_size() const noexcept;

  // script_window
  std::error_code draw_frame() override;
  std::error_code draw_for(std::chrono::milliseconds duration) override;

protected:
  // toplevel_window
  void resize(size sz) override;
  void close() override { closed = true; }
  void display(wl_output& disp) override;

private:
  event_loop& eloop_;
  egl::surface egl_surface_;
  wl::unique_ptr<wl_egl_window> egl_wnd_;
  std::optional<renderer> renderer_;
  bool closed = false;
};
