#include "task_guard.hpp"
#include "executors_environment.test.hpp"

#include <catch2/catch_test_macros.hpp>

#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

using namespace std::literals;

SCENARIO("task guard") {
  GIVEN("some worker function with active running instances counter") {
    std::atomic<int> active_works_count{0};
    auto work = [&active_works_count](std::stop_token tok) {
      ++active_works_count;
      active_works_count.notify_all();
      while (!tok.stop_requested())
        std::this_thread::sleep_for(5ms);
      --active_works_count;
    };

    WHEN("task guard created") {
      task_guard task{executors_environment::pool_executor(), work};

      THEN("it is not finished") { REQUIRE(task.is_finished() == false); }

      THEN("one worker is started") {
        active_works_count.wait(0);
        REQUIRE(active_works_count.load() == 1);
      }
    }

    WHEN("task guard destroyed") {
      {
        task_guard task{executors_environment::pool_executor(), work};
        active_works_count.wait(0);
      }
      THEN("no workers are running") { REQUIRE(active_works_count.load() == 0); }
    }

    WHEN("worker function stops") {
      task_guard task{executors_environment::pool_executor(), [](std::stop_token) {}};
      executors_environment::wait_pool_tasks_done();
      THEN("task is done") { REQUIRE(task.is_finished() == true); }
    }
  }
}
