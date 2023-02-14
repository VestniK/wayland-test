#include <span>

#include <fmt/format.h>
#include <spdlog/cfg/env.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/systemd_sink.h>
#include <spdlog/spdlog.h>

#include <asio/awaitable.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <asio/static_thread_pool.hpp>

#include <wayland/event_loop.hpp>
#include <wayland/gamepad/udev_gamepads.hpp>
#include <wayland/gles2/renderer.hpp>
#include <wayland/gles_window.hpp>
#include <wayland/util/get_option.hpp>

using namespace std::literals;

namespace {

void setup_logger() {
  auto journald =
      std::make_shared<spdlog::sinks::systemd_sink_mt>("wayland-test", true);
  spdlog::default_logger()->sinks() = {journald};
#if !defined(NDEBUG)
  auto term = std::make_shared<spdlog::sinks::stderr_color_sink_mt>(
      spdlog::color_mode::automatic);
  spdlog::default_logger()->sinks().push_back(term);
#endif
  spdlog::cfg::load_env_levels();
}

} // namespace

namespace co {

unsigned min_threads = 3;

asio::awaitable<int> main(asio::io_context::executor_type io_exec,
                          asio::thread_pool::executor_type pool_exec,
                          std::span<char *> args) {
  if (get_flag(args, "-h")) {
    fmt::print("Usage: {} [-d DISPLAY]\n"
               "\t-d DISPLAY\tSpecify wayland display. Current session default "
               "is used if nothing is specified.\n",
               args[0]);
    co_return EXIT_SUCCESS;
  }
  setup_logger();

  event_loop eloop{get_option(args, "-d")};
  registry reg{eloop};

  xdg_wm_base_listener xdg_listener{
      .ping = [](void *, xdg_wm_base *wm, uint32_t serial) {
        xdg_wm_base_pong(wm, serial);
      }};
  xdg_wm_base_add_listener(reg.get_xdg_wm(), &xdg_listener, nullptr);

  udev_gamepads gamepads;
  gamepads.list();

  auto wnd = co_await gles_window::create_maximized(
      eloop, reg, io_exec, pool_exec, make_render_func<scene_renderer>());

  using namespace asio::experimental::awaitable_operators;
  co_await (eloop.dispatch_while(io_exec, [&] {
    if (auto ec = reg.check()) {
      spdlog::error("Wayland services state error: {}", ec.message());
      return true;
    }
    return !wnd.is_closed();
  }) || gamepads.watch(io_exec));

  spdlog::debug("window is closed exit the app");
  co_return EXIT_SUCCESS;
}

} // namespace co
