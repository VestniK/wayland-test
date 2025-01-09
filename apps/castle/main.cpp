#include <iostream>
#include <span>

#include <spdlog/spdlog.h>

#include <asio/awaitable.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <asio/static_thread_pool.hpp>

#include <libs/cli/struct_args.hpp>

#include <apps/castle/draw_scene.hpp>

#include <libs/corort/executors.hpp>

using namespace std::literals;
using namespace asio::experimental::awaitable_operators;

namespace {

struct opts {
  const char* display = args::option<const char*>{"-d", "--display",
      "Specify wayland display. Current session default is used if nothing is "
      "specified."}
                            .default_value(nullptr);
};

} // namespace

namespace co {

unsigned min_threads = 3;

asio::awaitable<int> main(
    io_executor io_exec, pool_executor pool_exec, std::span<char*> args) {
  if (get_flag(args, "-h")) {
    args::usage<opts>(args[0], std::cout);
    std::cout << '\n';
    args::args_help<opts>(std::cout);
    co_return EXIT_SUCCESS;
  }
  const auto opt = args::parse<opts>(args);

  co_await draw_scene(io_exec, pool_exec, opt.display);

  co_return EXIT_SUCCESS;
}

} // namespace co
