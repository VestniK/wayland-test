#include <span>
#include <variant>

#include <fmt/format.h>

#include <asio/awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/io_service.hpp>

extern asio::awaitable<int> co_main(asio::io_context::executor_type exec,
                                    std::span<char *> args);

int main(int argc, char **argv) {
  asio::io_service io;

  std::variant<std::monostate, int, std::exception_ptr> rc;
  asio::co_spawn(io, co_main(io.get_executor(), {argv, argv + argc}),
                 [&rc](std::exception_ptr err, int ec) {
                   if (err)
                     rc = std::move(err);
                   else
                     rc = ec;
                 });
  io.run();

  switch (rc.index()) {
  case 0:
    fmt::print(stderr, "Interrupted\n");
    break;
  case 1:
    return std::get<int>(rc);
  case 2:
    std::rethrow_exception(std::get<std::exception_ptr>(rc));
  }
  return EXIT_FAILURE;
}
