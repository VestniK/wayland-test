#include <app/draw_scene.hpp>

#include <spdlog/spdlog.h>

#include <asio/co_spawn.hpp>

#include <util/xdg.hpp>

#include <img/load.hpp>

#include <gles2/renderer.hpp>
#include <vk/prepare_instance.hpp>

#include <wayland/event_loop.hpp>
#include <wayland/gles_window.hpp>
#include <wayland/gui_shell.hpp>

asio::awaitable<void> draw_scene(co::io_executor io_exec,
    co::pool_executor pool_exec, const scene::controller& controller,
    const char* wl_display) {
  event_loop eloop{wl_display};
  wl::gui_shell shell{eloop};

  struct : xdg::delegate {
    void close() override { closed = true; }
    void resize(size sz [[maybe_unused]]) override {}

    bool closed = false;
  } delegate;
  auto [vkwnd, sz] = co_await shell.create_maximized_window(eloop, io_exec);
  vkwnd.set_delegate(&delegate);
  prepare_instance(eloop.get_display(), vkwnd.get_surface(), sz);

  gles_window wnd{eloop, pool_exec,
      co_await shell.create_maximized_window(eloop, io_exec),
      make_render_func<scene_renderer>(std::cref(controller))};

  co_await eloop.dispatch_while(io_exec, [&] {
    if (auto ec = shell.check()) {
      spdlog::error("Wayland services state error: {}", ec.message());
      return false;
    }
    return !wnd.is_closed() && !delegate.closed;
  });

  spdlog::debug("window is closed exit the app");
}
