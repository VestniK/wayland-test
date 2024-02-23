#include "executors_environment.test.hpp"

#include <barrier>

#include <asio/post.hpp>

namespace {

constexpr size_t threads_count = 2;

}

std::unique_ptr<executors_environment::environment_data>
    executors_environment::environment_data_instance;

struct executors_environment::environment_data {
  std::barrier<> barrier{threads_count + 1};
  asio::static_thread_pool pool{threads_count};
};

void executors_environment::testRunStarting(const Catch::TestRunInfo&) {
  environment_data_instance = std::make_unique<environment_data>();
}

void executors_environment::testRunEnded(const Catch::TestRunStats&) {
  environment_data_instance.reset();
}

asio::static_thread_pool::executor_type executors_environment::pool_executor() {
  return environment_data_instance->pool.get_executor();
}

void executors_environment::wait_pool_tasks_done() {
  for (size_t i = 0; i < threads_count; ++i)
    asio::post(environment_data_instance->pool,
        [] { environment_data_instance->barrier.arrive_and_wait(); });
  environment_data_instance->barrier.arrive_and_wait();
}

CATCH_REGISTER_LISTENER(executors_environment)
