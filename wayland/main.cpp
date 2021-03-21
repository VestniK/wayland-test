#include <fmt/format.h>

#include <wayland/event_loop.hpp>
#include <wayland/gles_window.hpp>
#include <wayland/script_player.hpp>
#include <wayland/util/get_option.hpp>
#include <wayland/util/xdg.hpp>

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

  event_loop eloop{get_option(argc, argv, "-d")};
  gles_window wnd{eloop, 1337};

  while (!wnd.is_initialized())
    eloop.dispatch();

  play_script(get_option(argc, argv, "-s", xdg::find_config("wayland.chai")),
              wnd);

  return EXIT_SUCCESS;
}
