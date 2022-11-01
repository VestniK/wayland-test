#include <concepts>
#include <latch>
#include <optional>
#include <random>
#include <stop_token>
#include <utility>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include <asio/post.hpp>

#include "executors_environment.test.hpp"
#include "geom.hpp"

template <std::regular T> class mutex_value_update_channel {
public:
  std::optional<T> get_update() {
    const T val = (std::lock_guard{mutex_}, current_);
    if (val == prev_)
      return std::nullopt;
    return prev_ = val;
  }
  void update(const T &t) { std::lock_guard{mutex_}, current_ = t; }

private:
  std::mutex mutex_;
  T current_;
  T prev_;
};

template <std::regular T> class atomic_value_update_channel {
private:
  struct box {
    alignas(std::hardware_destructive_interference_size) T val;
  };

public:
  std::optional<T> get_update() {
    const T val = buf_[consumer_ & ~seen_mask].val;
    consumer_ = cur_.exchange(consumer_);
    if (consumer_ & seen_mask)
      return std::nullopt;
    consumer_ = consumer_ | seen_mask;
    if (val == buf_[consumer_ & ~seen_mask].val)
      return std::nullopt;
    return buf_[consumer_ & ~seen_mask].val;
  }
  void update(const T &t) {
    buf_[producer_].val = t;
    producer_ = cur_.exchange(producer_) & ~seen_mask;
  }

private:
  static constexpr size_t seen_mask = (1 << 3);

private:
  std::array<box, 3> buf_;
  std::atomic<size_t> cur_ = 0;
  size_t producer_ = 1;
  size_t consumer_ = 2 | seen_mask;
};

template <std::regular T>
using value_update_channel = atomic_value_update_channel<T>;

namespace {

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

SCENARIO("value update channel") {
  GIVEN("some channel") {
    value_update_channel<size> channel;
    WHEN("nothing updated") {
      THEN("no value is returned from get_update") {
        REQUIRE(channel.get_update() == std::nullopt);
      }
    }

    WHEN("value is updaed") {
      channel.update({100, 500});
      THEN("new value is returned from get_update") {
        REQUIRE(channel.get_update() == size{100, 500});
      }

      THEN("nothing is returned on second get_update call") {
        channel.get_update();
        REQUIRE(channel.get_update() == std::nullopt);
      }

      AND_WHEN("value is updated once again") {
        channel.update({42, 42});
        THEN("new value is returned from get_update") {
          REQUIRE(channel.get_update() == size{42, 42});
        }
      }
    }

    WHEN("value is updated twice without get_update in between") {
      channel.update({100, 500});
      channel.update({42, 42});
      THEN("the last value is returned from get_update") {
        REQUIRE(channel.get_update() == size{42, 42});
      }
    }

    WHEN("value is set in another thread") {
      std::latch latch{2};
      asio::post(executors_environment::pool_executor(), [&channel, &latch] {
        latch.arrive_and_wait();
        channel.update({100, 500});
      });

      THEN("get_update will eventually return the value set") {
        latch.arrive_and_wait();
        std::optional<size> val;
        while (!val)
          val = channel.get_update();
        REQUIRE(val == size{100, 500});
      }
    }
  }
}

TEST_CASE("value_update_channel consumer API benchmarks") {
  BENCHMARK_ADVANCED("check for empty update without thread contention")
  (Catch::Benchmark::Chronometer meter) {
    value_update_channel<int> channel;
    meter.measure([&] { return channel.get_update(); });
  };

  BENCHMARK_ADVANCED("check for update from another thread")
  (Catch::Benchmark::Chronometer meter) {
    value_update_channel<int> channel;

    std::stop_source stop;
    std::latch start{2};
    asio::post(executors_environment::pool_executor(),
               [&channel, vals = make_some_nums(4096), &start,
                stop = stop.get_token()] {
                 start.arrive_and_wait();
                 while (!stop.stop_requested()) {
                   for (int v : vals)
                     channel.update(v);
                 }
               });

    start.arrive_and_wait();
    meter.measure([&] { return channel.get_update(); });

    stop.request_stop();
    executors_environment::wait_pool_tasks_done();
  };
}
