#pragma once

#include <spdlog/spdlog.h>

#include <libs/eglctx/egl.hpp>
#include <libs/sync/channel.hpp>

#include <libs/wlwnd/renderer.hpp>
#include <libs/wlwnd/vsync_frames.hpp>
#include <libs/wlwnd/wlutil.hpp>

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

template <renderer Renderer, typename... A>
  requires std::constructible_from<Renderer, A...>
animation_function make_gles_animation_function(A&&... a) {
  return [... args = std::forward<A>(a)](wl_display& display, wl_surface& surf,
             vsync_frames& frames,
             value_update_channel<size>& resize_channel) mutable {
    gles_context ctx(display, surf, resize_channel.get_current());
    spdlog::debug("OpenGL ES2 context created");

    ctx.egl_surface().swap_buffers();
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
    spdlog::debug("rendering finished");
  };
}
