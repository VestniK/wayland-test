#include <span>
#include <variant>

#include <fmt/format.h>

#include <asio/awaitable.hpp>
#include <asio/posix/stream_descriptor.hpp>
#include <asio/use_awaitable.hpp>

#include <wayland/event_loop.hpp>
#include <wayland/gles_window.hpp>
#include <wayland/script_player.hpp>
#include <wayland/util/get_option.hpp>
#include <wayland/util/xdg.hpp>

asio::awaitable<void> process_events(asio::io_context::executor_type exec,
                                     event_loop &eloop) {
  if (wl_display_prepare_read(&eloop.get_display()) != 0) {
    eloop.dispatch_pending();
    co_return;
  }
  wl_display_flush(&eloop.get_display());
  asio::posix::stream_descriptor conn{exec,
                                      wl_display_get_fd(&eloop.get_display())};
  co_await conn.async_wait(asio::posix::stream_descriptor::wait_read,
                           asio::use_awaitable);
  conn.release();
  wl_display_read_events(&eloop.get_display());
  // TODO `wl_display_cancel_read(display);` on wait failure!!!
  eloop.dispatch_pending();
  co_return;
}

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
    co_await process_events(exec, eloop);
  }

  for (int i = 0; i < 1000; ++i) {
    wnd.camera_look_at(7, 10, 18, 3 + i * 0.007, 1 + i * 0.005, 0);
    wnd.draw_frame();
  }

  co_return EXIT_SUCCESS;
}
