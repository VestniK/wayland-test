#include <span>

#include <fmt/format.h>
#include <spdlog/cfg/env.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/systemd_sink.h>
#include <spdlog/spdlog.h>

#include <asio/awaitable.hpp>

#include <wayland/event_loop.hpp>
#include <wayland/gamepad/udev_gamepads.hpp>
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

asio::awaitable<int> co_main(asio::io_context::executor_type exec,
                             std::span<char *> args) {
  if (get_flag(args, "-h")) {
    fmt::print("Usage: {} [-d DISPLAY]\n"
               "\t-d DISPLAY\tSpecify wayland display. Current session default "
               "is used if nowhing is specified.\n",
               args[0]);
    co_return EXIT_SUCCESS;
  }
  setup_logger();

  udev_gamepads gamepads;
  gamepads.watch(exec);
  gamepads.list();

  event_loop eloop{get_option(args, "-d")};

  auto wnd = co_await gles_window::create(eloop, exec);
  while (!wnd.is_closed())
    co_await eloop.dispatch(exec);

  co_return EXIT_SUCCESS;
}
