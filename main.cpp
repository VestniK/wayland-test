#include <fstream>
#include <span>

#include <fmt/format.h>
#include <spdlog/cfg/env.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/systemd_sink.h>
#include <spdlog/spdlog.h>

#include <asio/awaitable.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <asio/experimental/promise.hpp>
#include <asio/experimental/use_promise.hpp>
#include <asio/static_thread_pool.hpp>

#include <util/get_option.hpp>
#include <util/xdg.hpp>

#include <gamepad/udev_gamepads.hpp>

#include <gles2/renderer.hpp>

#include <xdg-shell.h>

#include <wayland/event_loop.hpp>
#include <wayland/gles_window.hpp>
#include <wayland/gui_shell.hpp>

#include <img/load.hpp>

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

asio::awaitable<img::image> load(std::filesystem::path path) {
  std::ifstream in{path};
  if (!in)
    throw std::system_error{errno, std::system_category(), "open"};
  co_return img::load(in);
}

class controller : public scene_renderer::controller {
public:
  controller() : gamepads_{std::ref(*this), axes_} {
    axes_.set_axis_channel(gamepad::axis::main, cube_vel_);
    cube_tex_offset_update_.update({.dest = {}, .duration = 0ms});
  }

  void operator()(gamepad::key key, bool pressed) {
    if (pressed)
      return;
    switch (key) {
    case gamepad::key::A:
      cube_tex_offset_update_.update({.dest = {.0, .0}, .duration = 600ms});
      break;
    case gamepad::key::B:
      cube_tex_offset_update_.update({.dest = {.5, .0}, .duration = 600ms});
      break;
    case gamepad::key::X:
      cube_tex_offset_update_.update({.dest = {.0, .5}, .duration = 600ms});
      break;
    case gamepad::key::Y:
      cube_tex_offset_update_.update({.dest = {.5, .5}, .duration = 600ms});
      break;

    case gamepad::key::left_trg:
    case gamepad::key::right_trg:
    case gamepad::key::left_alt_trg:
    case gamepad::key::right_alt_trg:
    case gamepad::key::select:
    case gamepad::key::start:
    case gamepad::key::dpad_down:
    case gamepad::key::dpad_up:
    case gamepad::key::dpad_left:
    case gamepad::key::dpad_right:
      break;
    }
  }

  asio::experimental::promise<void(std::exception_ptr)> start(
      asio::io_context::executor_type io_exec) {
    return asio::co_spawn(
        io_exec, gamepads_.watch(io_exec), asio::experimental::use_promise);
  }

private:
  evdev_gamepad::axes_state axes_;
  udev_gamepads gamepads_;
};

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

  auto cube_tex =
      asio::co_spawn(pool_exec, load(xdg::find_data("resources/cube-tex.png")),
          asio::experimental::use_promise);
  auto land_tex =
      asio::co_spawn(pool_exec, load(xdg::find_data("resources/grass-tex.png")),
          asio::experimental::use_promise);

  event_loop eloop{get_option(args, "-d")};
  wl::gui_shell shell{eloop};

  controller contr;
  auto contr_coro = contr.start(io_exec);

  gles_window wnd{eloop, pool_exec,
      co_await shell.create_maximized_window(eloop, io_exec),
      make_render_func<scene_renderer>(co_await std::move(cube_tex),
          co_await std::move(land_tex), std::cref(contr))};

  co_await eloop.dispatch_while(io_exec, [&] {
    if (auto ec = shell.check()) {
      spdlog::error("Wayland services state error: {}", ec.message());
      return false;
    }
    return !wnd.is_closed();
  });

  spdlog::debug("window is closed exit the app");
  co_return EXIT_SUCCESS;
}

} // namespace co
