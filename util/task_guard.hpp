#pragma once

#include <latch>
#include <stop_token>

#include <asio/post.hpp>
#include <asio/static_thread_pool.hpp>

class task_guard {
public:
  template <typename F>
  requires std::is_invocable_v<F, std::stop_token>
  task_guard(asio::static_thread_pool::executor_type exec, F &&task) {
    asio::post(exec,
               [task = std::forward<F>(task), tok = stop_sorce_.get_token(),
                &done_flag = done_flag_]() mutable {
                 task(std::move(tok));
                 done_flag.count_down();
               });
  }

  ~task_guard() noexcept {
    stop_sorce_.request_stop();
    done_flag_.wait();
  }

  bool is_finished() const noexcept { return done_flag_.try_wait(); }
  bool is_finishing() const noexcept { return stop_sorce_.stop_requested(); }
  void stop() { stop_sorce_.request_stop(); }
  std::stop_token stop_token() { return stop_sorce_.get_token(); }

private:
  std::stop_source stop_sorce_;
  std::latch done_flag_{1};
};
