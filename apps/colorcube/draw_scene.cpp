#include "draw_scene.hpp"

#include <spdlog/spdlog.h>

#include <asio/co_spawn.hpp>

#include <libs/eglctx/gles_context.hpp>
#include <libs/img/load.hpp>
#include <libs/xdg/xdg.hpp>

#include <apps/colorcube/renderer.hpp>

#include <libs/wlwnd/animation_window.hpp>
#include <libs/wlwnd/event_loop.hpp>
#include <libs/wlwnd/gui_shell.hpp>

asio::awaitable<void> draw_scene(
    co::io_executor io_exec, co::pool_executor pool_exec, const scene::controller& controller,
    const char* wl_display
) {
  event_loop eloop{wl_display};
  wl::gui_shell shell{eloop};

  animation_window wnd{
      eloop.make_queue(), pool_exec, co_await shell.create_maximized_window(eloop, io_exec),
      make_gles_animation_function<scene_renderer>(std::cref(controller))
  };

  co_await eloop.dispatch_while(io_exec, [&] {
    if (auto ec = shell.check()) {
      spdlog::error("Wayland services state error: {}", ec.message());
      return false;
    }
    return !wnd.is_closed();
  });

  spdlog::debug("window is closed exit the app");
}
