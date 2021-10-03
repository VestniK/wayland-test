#include <span>
#include <variant>

#include <fmt/format.h>

#include <asio/awaitable.hpp>

#include <wayland/event_loop.hpp>
#include <wayland/gles_window.hpp>
#include <wayland/script_player.hpp>
#include <wayland/util/get_option.hpp>
#include <wayland/util/xdg.hpp>

using namespace std::literals;

asio::awaitable<int> co_main(asio::io_context::executor_type exec,
                             std::span<char *> args) {
  if (get_flag(args, "-h")) {
    fmt::print("Usage: {} [-d DISPLAY] [-s SCRIPT_PATH]\n"
               "\t-d DISPLAY\tSpecify wayland display. Current session default "
               "is used if nowhing is specified.\n",
               args[0]);
    co_return EXIT_SUCCESS;
  }

  event_loop eloop{get_option(args, "-d")};
  gles_window wnd{eloop, 1337};

  while (!wnd.is_initialized()) {
    co_await eloop.dispatch(exec);
  }

  wnd.camera_look_at(7, 10, 18, 3, 1, 0);
  wnd.draw_for(25s);

  co_return EXIT_SUCCESS;
}
