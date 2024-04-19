#pragma once

#include <memory>

#include <asio/static_thread_pool.hpp>

#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

class executors_environment : public Catch::EventListenerBase {
public:
  using Catch::EventListenerBase::EventListenerBase;

  void testRunStarting(const Catch::TestRunInfo&) override;
  void testRunEnded(const Catch::TestRunStats&) override;

  static asio::static_thread_pool::executor_type pool_executor();
  static void wait_pool_tasks_done();

private:
  struct environment_data;

  static std::unique_ptr<environment_data> environment_data_instance;
};
