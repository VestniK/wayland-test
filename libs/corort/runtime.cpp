#include <filesystem>
#include <span>
#include <thread>
#include <variant>

#include <fmt/format.h>

#include <asio/awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/io_service.hpp>
#include <asio/static_thread_pool.hpp>

#include <spdlog/cfg/env.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/systemd_sink.h>
#include <spdlog/spdlog.h>

#include <libs/corort/executors.hpp>

namespace {

void setup_logger(const std::string& app_name) {
  auto journald = std::make_shared<spdlog::sinks::systemd_sink_mt>(app_name, true);
  spdlog::default_logger()->sinks() = {journald};
#if !defined(NDEBUG)
  auto term = std::make_shared<spdlog::sinks::stderr_color_sink_mt>(spdlog::color_mode::automatic);
  spdlog::default_logger()->sinks().push_back(term);
#endif
  spdlog::cfg::load_env_levels();
}

} // namespace

namespace co {

extern asio::awaitable<int> main(io_executor io_exec, pool_executor pool_exec, std::span<char*> args);
extern unsigned min_threads;

} // namespace co

int main(int argc, char** argv) {
  setup_logger(std::filesystem::path{argv[0]}.filename().string());

  asio::io_service io;
  asio::static_thread_pool pool{std::max(co::min_threads, std::thread::hardware_concurrency()) - 1};

  std::variant<std::monostate, int, std::exception_ptr> rc;
  asio::co_spawn(
      io, co::main(io.get_executor(), pool.get_executor(), {argv, argv + argc}),
      [&rc](std::exception_ptr err, int ec) {
        if (err)
          rc = std::move(err);
        else
          rc = ec;
      }
  );
  asio::post(pool, [&io, &pool] {
    io.run();
    pool.stop();
  });
  pool.attach();
  pool.wait();

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
