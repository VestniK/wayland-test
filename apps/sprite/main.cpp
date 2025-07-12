#include <iostream>
#include <span>

#include <asio/awaitable.hpp>

#include <spdlog/spdlog.h>

#include <wayland-client.h>

#include <thinsys/io/io.hpp>
#include <thinsys/io/mmap.hpp>

#include <libs/cli/struct_args.hpp>
#include <libs/corort/executors.hpp>
#include <libs/img/load.hpp>
#include <libs/sfx/sfx.hpp>
#include <libs/wlwnd/animation_window.hpp>
#include <libs/wlwnd/event_loop.hpp>
#include <libs/wlwnd/framebuf.hpp>
#include <libs/wlwnd/gui_shell.hpp>
#include <libs/wlwnd/vsync_frames.hpp>

namespace {

struct opts {
  const char* display =
      args::option<const char*>{
          "-d", "--display",
          "Specify wayland display. Current session default is used if nothing is specified."
      }
          .default_value(nullptr);
};

animation_function make_animation_function(wl_shm& shm, img::image<img::pixel_fmt::rgba> img) {
  return [&shm, img = std::move(img)](
             wl_display& display, wl_surface& surf, vsync_frames& frames,
             value_update_channel<size>& resize_channel
         ) {
    const size sz = resize_channel.get_current();

    wl::framebuf fb{shm, sz};
    std::ranges::copy(img.bytes(), fb.front().data());
    fb.swap(surf);

    for (auto ts [[maybe_unused]] : frames) {
      if (const auto sz = resize_channel.get_update()) {
        fb = wl::framebuf{shm, sz.value()};
      }

      std::ranges::copy(img.bytes(), fb.front().data());
      fb.swap(surf);
    }
  };
}

} // namespace

namespace co {

unsigned min_threads = 3;

asio::awaitable<int> main(io_executor io_exec, pool_executor pool_exec, std::span<char*> args) {
  if (get_flag(args, "-h")) {
    args::usage<opts>(args[0], std::cout);
    std::cout << '\n';
    args::args_help<opts>(std::cout);
    co_return EXIT_SUCCESS;
  }
  const auto opt = args::parse<opts>(args);

  event_loop eloop{opt.display};
  wl::gui_shell shell{eloop};

  auto res = sfx::archive::open_self();
  auto& fd = res.open("images/head.png");
  auto img = img::load<img::pixel_fmt::rgba>(fd);
  const size sz = img.size();

  animation_window wnd{
      eloop.make_queue(), pool_exec, shell.create_window(eloop, sz),
      make_animation_function(*shell.get_shm(), std::move(img))
  };

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
