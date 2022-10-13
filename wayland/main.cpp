#include <span>

#include <fmt/format.h>

#include <asio/awaitable.hpp>

#include <wayland/event_loop.hpp>
#include <wayland/gles_window.hpp>
#include <wayland/util/get_option.hpp>

using namespace std::literals;

asio::awaitable<int> co_main(asio::io_context::executor_type exec,
                             std::span<char *> args) {
  if (get_flag(args, "-h")) {
    fmt::print("Usage: {} [-d DISPLAY]\n"
               "\t-d DISPLAY\tSpecify wayland display. Current session default "
               "is used if nowhing is specified.\n",
               args[0]);
    co_return EXIT_SUCCESS;
  }

  event_loop eloop{get_option(args, "-d")};

  auto wnd = co_await gles_window::create(eloop, exec);
  while (!wnd.is_closed())
    co_await eloop.dispatch(exec);

  co_return EXIT_SUCCESS;
}
