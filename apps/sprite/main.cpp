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
#include <libs/wlwnd/gui_shell.hpp>
#include <libs/wlwnd/vsync_frames.hpp>
#include <libs/xdg/xdg.hpp>

namespace {

struct opts {
  const char* display =
      args::option<const char*>{
          "-d", "--display",
          "Specify wayland display. Current session default is used if nothing is specified."
      }
          .default_value(nullptr);
};

animation_function make_animation_function(wl_buffer* buf) {
  return [buf](
             wl_display& display, wl_surface& surf, vsync_frames& frames,
             value_update_channel<size>& resize_channel
         ) {
    const size sz = resize_channel.get_current();

    wl_surface_damage(&surf, 0, 0, sz.width, sz.height);
    wl_surface_attach(&surf, buf, 0, 0);
    wl_surface_commit(&surf);
    for (auto ts [[maybe_unused]] : frames) {
      if (const auto sz = resize_channel.get_update()) {
      }
      wl_surface_damage(&surf, 0, 0, sz.width, sz.height);
      wl_surface_attach(&surf, buf, 0, 0);
      wl_surface_commit(&surf);
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
  auto reader = img::load_reader(fd);
  assert(reader.format() == img::pixel_fmt::rgba);
  const size sz = reader.size();
  const size_t pixel_size = 4 * sz.width * sz.height;

  auto mapping_fd = thinsys::io::open_anonymous(xdg::runtime_dir(), thinsys::io::mode::read_write);
  thinsys::io::truncate(mapping_fd, pixel_size);
  wl::unique_ptr<wl_shm_pool> spool{
      wl_shm_create_pool(shell.get_shm(), mapping_fd.native_handle(), pixel_size)
  };

  auto mapping = thinsys::io::mmap_mut(mapping_fd, 0, pixel_size, thinsys::io::map_sharing::sahre);
  reader.read_pixels(std::span{data(mapping), thinsys::io::size(mapping)});

  wl::unique_ptr<wl_buffer> buf{
      wl_shm_pool_create_buffer(spool.get(), 0, sz.width, sz.height, 4 * sz.width, WL_SHM_FORMAT_ARGB8888)
  };

  animation_window wnd{
      eloop.make_queue(), pool_exec, shell.create_window(eloop, reader.size()),
      make_animation_function(buf.get())
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
