#include <fmt/format.h>

#include <asio/awaitable.hpp>
#include <asio/io_service.hpp>
#include <asio/posix/stream_descriptor.hpp>
#include <asio/use_awaitable.hpp>

#include <wayland/event_loop.hpp>
#include <wayland/gles_window.hpp>
#include <wayland/script_player.hpp>
#include <wayland/util/get_option.hpp>
#include <wayland/util/xdg.hpp>

template <typename Exec, typename Pred>
asio::awaitable<void> async_wait(const Exec &exec, event_loop &eloop,
                                 Pred &&pred) {
  asio::posix::stream_descriptor conn{exec,
                                      wl_display_get_fd(&eloop.get_display())};
  while (!pred()) {
    co_await conn.async_wait(asio::posix::stream_descriptor::wait_read,
                             asio::use_awaitable);
    eloop.dispatch_pending();
  }
}

int main(int argc, char **argv) {
  if (get_flag(argc, argv, "-h")) {
    fmt::print("Usage: {} [-d DISPLAY] [-s SCRIPT_PATH]\n"
               "\t-d DISPLAY\tSpecify wayland display. Current session default "
               "is used if nowhing is specified.\n"
               "\t-s SCRIPT_PATH\tPath to the script to play. XDG config "
               "wayland.chai is searched if nothing specified\n",
               argv[0]);
    return EXIT_SUCCESS;
  }

  asio::io_service io;
  event_loop eloop{get_option(argc, argv, "-d")};
  gles_window wnd{eloop, 1337};

  {
    auto task = async_wait(io.get_executor(), eloop,
                           [&wnd] { return wnd.is_initialized(); });
    io.run();
  }

  play_script(get_option(argc, argv, "-s", xdg::find_config("wayland.chai")),
              wnd);

  return EXIT_SUCCESS;
}
