#include "draw_scene.hpp"

#include <spdlog/spdlog.h>

#include <asio/co_spawn.hpp>

#include <libs/xdg/xdg.hpp>

#include <libs/wlwnd/animation_window.hpp>
#include <libs/wlwnd/event_loop.hpp>
#include <libs/wlwnd/gui_shell.hpp>
#include <libs/wlwnd/vsync_frames.hpp>

#include <apps/castle/prepare_instance.hpp>

static_assert(renderer<renderer_iface>);

animation_function make_vk_animation_function() {
  return [](wl_display& display, wl_surface& surf, vsync_frames& frames,
             value_update_channel<size>& resize_channel) {
    auto render = make_vk_renderer(display, surf, resize_channel.get_current());
    render->draw({});
    for (auto ts : frames) {
      if (const auto sz = resize_channel.get_update()) {
        render->resize(sz.value());
      }

      render->draw(ts);
    }
  };
}

asio::awaitable<void> draw_scene(co::io_executor io_exec,
    co::pool_executor pool_exec, const char* wl_display) {
  event_loop eloop{wl_display};
  wl::gui_shell shell{eloop};

  animation_window wnd{eloop.make_queue(), pool_exec,
      co_await shell.create_maximized_window(eloop, io_exec),
      make_vk_animation_function()};

  co_await eloop.dispatch_while(io_exec, [&] {
    if (auto ec = shell.check()) {
      spdlog::error("Wayland services state error: {}", ec.message());
      return false;
    }
    return !wnd.is_closed();
  });

  spdlog::debug("window is closed exit the app");
}
