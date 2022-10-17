#include <span>

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <spdlog/cfg/env.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/systemd_sink.h>
#include <spdlog/spdlog.h>

#include <asio/awaitable.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <asio/static_thread_pool.hpp>

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

template <typename Pred>
asio::awaitable<void> dispatch_while(asio::io_context::executor_type exec,
                                     event_loop &eloop, Pred &&pred) {
  while (pred())
    co_await eloop.dispatch_once(exec);
  co_return;
}

} // namespace

unsigned min_threads = 3;

asio::awaitable<int> co_main(asio::io_context::executor_type io_exec,
                             asio::thread_pool::executor_type pool_exec,
                             std::span<char *> args) {
  if (get_flag(args, "-h")) {
    fmt::print("Usage: {} [-d DISPLAY]\n"
               "\t-d DISPLAY\tSpecify wayland display. Current session default "
               "is used if nowhing is specified.\n",
               args[0]);
    co_return EXIT_SUCCESS;
  }
  setup_logger();

  event_loop eloop{get_option(args, "-d")};

  auto wnd = co_await gles_window::create(eloop, io_exec);

  udev_gamepads gamepads;
  gamepads.list();

  using namespace asio::experimental::awaitable_operators;
  co_await (dispatch_while(io_exec, eloop, [&] { return !wnd.is_closed(); }) ||
            gamepads.watch(io_exec));

  co_return EXIT_SUCCESS;
}
