#include <span>

#include <fmt/format.h>
#include <spdlog/cfg/env.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/systemd_sink.h>
#include <spdlog/spdlog.h>

#include <asio/awaitable.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <asio/static_thread_pool.hpp>

#include <util/get_option.hpp>

#include <gamepad/udev_gamepads.hpp>

#include <gles2/renderer.hpp>

#include <xdg-shell.h>

#include <wayland/event_loop.hpp>
#include <wayland/gles_window.hpp>
#include <wayland/gui_shell.hpp>

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
    asio::thread_pool::executor_type pool_exec, std::span<char*> args) {
  if (get_flag(args, "-h")) {
    fmt::print("Usage: {} [-d DISPLAY]\n"
               "\t-d DISPLAY\tSpecify wayland display. Current session default "
               "is used if nothing is specified.\n",
        args[0]);
    co_return EXIT_SUCCESS;
  }
  setup_logger();

  event_loop eloop{get_option(args, "-d")};
  wl::gui_shell shell{eloop};

  value_update_channel<glm::vec3> cube_color;
  value_update_channel<glm::vec3> landscale_color;
  udev_gamepads gamepads{
      [&cube_color, &landscale_color](gamepad_key key, bool pressed) {
        if (pressed)
          return;
        switch (key) {
        case gamepad_key::A:
          cube_color.update({.0, 0.9, 0.0});
          break;
        case gamepad_key::B:
          cube_color.update({.9, 0.0, 0.0});
          break;
        case gamepad_key::X:
          cube_color.update({.0, .0, .9});
          break;
        case gamepad_key::Y:
          cube_color.update({0.8, .8, 0.4});
          break;
        case gamepad_key::left_trg:
          landscale_color.update({.8, .0, .0});
          break;
        case gamepad_key::right_trg:
          landscale_color.update({.0, .8, .0});
          break;
        case gamepad_key::left_alt_trg:
          landscale_color.update({.4, .0, .0});
          break;
        case gamepad_key::right_alt_trg:
          landscale_color.update({.0, .4, .0});
          break;
        case gamepad_key::select:
          cube_color.update({.2, .2, .2});
          break;
        case gamepad_key::start:
          cube_color.update({.9, 0.7, 0.7});
          landscale_color.update({1., 1., 0.4});
          break;

        case gamepad_key::dpad_down:
        case gamepad_key::dpad_up:
        case gamepad_key::dpad_left:
        case gamepad_key::dpad_right:
          break;
        }
      }};
  auto gamepads_watch_result = gamepads.watch(io_exec);

  gles_window wnd{eloop, pool_exec,
      co_await shell.create_maximized_window(eloop, io_exec),
      make_render_func<scene_renderer>(
          std::ref(cube_color), std::ref(landscale_color))};

  using namespace asio::experimental::awaitable_operators;
  co_await (eloop.dispatch_while(io_exec, [&] {
    if (auto ec = shell.check()) {
      spdlog::error("Wayland services state error: {}", ec.message());
      return false;
    }
    return !wnd.is_closed();
  }) || std::move(gamepads_watch_result));

  spdlog::debug("window is closed exit the app");
  co_return EXIT_SUCCESS;
}

} // namespace co
