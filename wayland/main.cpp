#include <span>
#include <variant>

#include <fmt/format.h>

#include <asio/awaitable.hpp>

#include <wayland/event_loop.hpp>
#include <wayland/gles_window.hpp>
#include <wayland/ivi_window.hpp>
#include <wayland/script_player.hpp>
#include <wayland/util/get_option.hpp>
#include <wayland/util/xdg.hpp>
#include <wayland/xdg_window.hpp>

using namespace std::literals;

constexpr uint32_t ivi_main_glapp_id = 1337;

struct wait_size_delegate : xdg::delegate {
  void resize(size sz) override { wnd_size = sz; }
  void close() override { closed = true; }

  std::optional<size> wnd_size;
  bool closed = false;
};

using any_window = std::variant<xdg::toplevel_window, ivi::window>;

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

  wait_size_delegate szdelegate;
  any_window wnd =
      eloop.get_ivi()
          ? any_window{std::in_place_type<ivi::window>, *eloop.get_compositor(),
                       *eloop.get_ivi(), ivi_main_glapp_id, &szdelegate}
          : any_window{std::in_place_type<xdg::toplevel_window>,
                       *eloop.get_compositor(), *eloop.get_xdg_wm(),
                       &szdelegate};
  if (auto *xdg_wnd = std::get_if<xdg::toplevel_window>(&wnd))
    xdg_wnd->maximize();
  while (!szdelegate.wnd_size && !szdelegate.closed)
    co_await eloop.dispatch(exec);
  if (szdelegate.closed)
    co_return EXIT_SUCCESS;

  gles_delegate gl_delegate{
      eloop,
      std::visit([](auto &wnd) -> wl_surface & { return wnd.get_surface(); },
                 wnd),
      *szdelegate.wnd_size};
  std::visit([&](auto &wnd) { wnd.set_delegate(&gl_delegate); }, wnd);
  gl_delegate.camera_look_at(7, 10, 18, 3, 1, 0);
  gl_delegate.draw_for(25s);

  co_return EXIT_SUCCESS;
}
