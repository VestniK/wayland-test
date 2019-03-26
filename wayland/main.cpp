#include <iostream>

#include <wayland/event_loop.hpp>
#include <wayland/get_option.hpp>
#include <wayland/gles_window.hpp>
#include <wayland/script_player.hpp>
#include <wayland/xdg.hpp>

int main(int argc, char** argv) {
  if (has_flag(argc, argv, "-h")) {
    std::cout << "Ussage: " << argv[0] << " [-d DISPLAY] [-s SCRIPT_PATH]\n";
    std::cout << "\t-d DISPLAY\tSpecify wayland display. Current session "
                 "default is used if nowhing is specified.\n";
    std::cout << "\t-s SCRIPT_PATH\tPath to the script to play. XDG config "
                 "wayland.chai is searched if nothing specified\n";
    return EXIT_SUCCESS;
  }

  event_loop eloop{get_option(argc, argv, "-d")};
  gles_window wnd{eloop};

  wnd.maximize();
  while (!wnd.is_closed() && !wnd.is_initialized())
    eloop.dispatch();

  play_script(get_option(argc, argv, "-s", xdg::find_config("wayland.chai")), wnd);

  return EXIT_SUCCESS;
}
