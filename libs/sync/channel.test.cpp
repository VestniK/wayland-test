#include <latch>
#include <random>
#include <stop_token>
#include <utility>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <fmt/format.h>

#include <asio/post.hpp>

#include <libs/geom/geom.hpp>

#include "channel.hpp"
#include "executors_environment.test.hpp"

namespace {

template <std::regular T>
class mutex_value_update_channel {
public:
  mutex_value_update_channel() noexcept = default;
  explicit mutex_value_update_channel(const T& init) noexcept(std::is_nothrow_copy_constructible_v<T>) {
    update(init);
  }

  std::optional<T> get_update() {
    const T val = (std::lock_guard{mutex_}, current_);
    if (val == prev_)
      return std::nullopt;
    return prev_ = val;
  }
  T get_current() { return std::lock_guard{mutex_}, prev_ = current_; }
  void update(const T& t) { std::lock_guard{mutex_}, current_ = t; }

private:
  std::mutex mutex_;
  T current_;
  T prev_;
};

std::vector<int> make_some_nums(size_t count) {
  std::mt19937 gen;
  std::uniform_int_distribution<int> dist{100, 100500};
  std::vector<int> vals;
  vals.reserve(count);
  while (count-- > 0)
    vals.push_back(dist(gen));
  return vals;
}

} // namespace

namespace Catch {

template <>
struct StringMaker<size> {
  static std::string convert(size val) {
    return fmt::format("{{.width={}, .height={}}}", val.width, val.height);
  }
};

} // namespace Catch

TEMPLATE_TEST_CASE("value_update_channel", "", mutex_value_update_channel<size>, value_update_channel<size>) {
  GIVEN("some channel") {
    TestType channel;
    WHEN("nothing updated") {
      THEN("no value is returned from get_update") { REQUIRE(channel.get_update() == std::nullopt); }

      THEN("default constructed value is returned by current") { REQUIRE(channel.get_current() == size{}); }
    }

    WHEN("value is updaed") {
      channel.update({100, 500});
      THEN("new value is returned from get_update") { REQUIRE(channel.get_update() == size{100, 500}); }

      THEN("new value is returned from get_current") { REQUIRE(channel.get_current() == size{100, 500}); }

      THEN("same value is returned on second get_current call") {
        channel.get_current();
        REQUIRE(channel.get_current() == size{100, 500});
      }

      THEN("same value is returned on third get_current call") {
        channel.get_current();
        channel.get_current();
        REQUIRE(channel.get_current() == size{100, 500});
      }

      THEN("nothing is returned on second get_update call") {
        channel.get_update();
        REQUIRE(channel.get_update() == std::nullopt);
      }

      THEN("nothing is returned on call get_update after get_current") {
        channel.get_current();
        REQUIRE(channel.get_update() == std::nullopt);
      }

      THEN("nothing is returned on call get_update after multiple get_current") {
        channel.get_current();
        channel.get_current();
        REQUIRE(channel.get_update() == std::nullopt);
      }

      THEN("new value is returned on call get_current after get_update") {
        channel.get_update();
        REQUIRE(channel.get_current() == size{100, 500});
      }

      THEN("new value is returned on call get_current after multiple "
           "get_update") {
        channel.get_update();
        channel.get_update();
        REQUIRE(channel.get_current() == size{100, 500});
      }

      AND_WHEN("value is updated once again") {
        channel.update({42, 42});
        THEN("new value is returned from get_update") { REQUIRE(channel.get_update() == size{42, 42}); }

        THEN("new value is returned from get_current") { REQUIRE(channel.get_current() == size{42, 42}); }
      }
    }

    WHEN("value is updated twice without get_update in between") {
      channel.update({100, 500});
      channel.update({42, 42});
      THEN("the last value is returned from get_update") { REQUIRE(channel.get_update() == size{42, 42}); }

      THEN("the last value is returned from get_current") { REQUIRE(channel.get_update() == size{42, 42}); }
    }

    WHEN("value is set in another thread") {
      std::latch latch{2};
      asio::post(executors_environment::pool_executor(), [&channel, &latch] {
        latch.arrive_and_wait();
        channel.update({100, 500});
      });

      THEN("get_update will eventually return the value set") {
        std::optional<size> val;
        latch.arrive_and_wait();
        while (!val)
          val = channel.get_update();
        REQUIRE(val == size{100, 500});
      }

      THEN("get_current will eventually return the value set") {
        size val;
        latch.arrive_and_wait();
        while (val != size{100, 500})
          val = channel.get_current();
        REQUIRE(val == size{100, 500});
      }
    }
  }

  GIVEN("Some channel with initial value") {
    TestType channel{size{100, 500}};
    THEN("channel is in updated state with initial value") {
      REQUIRE(channel.get_update() == size{100, 500});
    }
    THEN("current value is equal to initial one") { REQUIRE(channel.get_current() == size{100, 500}); }

    WHEN("initial update is received") {
      channel.get_update();
      THEN("channel is not in updagetd state anymore") { REQUIRE(channel.get_update() == std::nullopt); }

      THEN("current value remains the same") { REQUIRE(channel.get_current() == size{100, 500}); }
    }

    WHEN("new value is set") {
      channel.update(size{42, 42});
      THEN("channel is in updated state with new value") { REQUIRE(channel.get_update() == size{42, 42}); }

      THEN("current value is the new one") { REQUIRE(channel.get_current() == size{42, 42}); }
    }
  }
}

namespace {

class bench_producer {
public:
  template <typename Channel>
  void start(Channel& channel) {
    asio::post(
        executors_environment::pool_executor(),
        [&channel, vals = make_some_nums(0xffffff), &start = start_, stop = stop_.get_token()] {
          start.arrive_and_wait();
          do {
            for (int v : vals)
              channel.update(v);
          } while (!stop.stop_requested());
        }
    );
    start_.arrive_and_wait();
  }

  ~bench_producer() noexcept {
    stop_.request_stop();
    executors_environment::wait_pool_tasks_done();
  }

private:
  std::stop_source stop_;
  std::latch start_{2};
};

} // namespace

TEMPLATE_TEST_CASE("value_update_channel consumer API benchmarks", "[!benchmark]", mutex_value_update_channel<int>, value_update_channel<int>) {
  TestType channel;

  BENCHMARK_ADVANCED("check for empty update without thread contention")
  (Catch::Benchmark::Chronometer meter) {
    meter.measure([&] { return channel.get_update(); });
  };

  bench_producer producer;
  producer.start(channel);
  BENCHMARK_ADVANCED("get_update with high thread contention")
  (Catch::Benchmark::Chronometer meter) {
    meter.measure([&] { return channel.get_update(); });
  };

  BENCHMARK_ADVANCED("get_current with high thread contention")
  (Catch::Benchmark::Chronometer meter) {
    meter.measure([&] { return channel.get_current(); });
  };
}
