#pragma once

#include <atomic>
#include <cstdint>
#include <utility>

class heartbeat;

class heartbeat_waiter {
public:
  constexpr heartbeat_waiter() noexcept = default;

  uint32_t wait_new_beats() noexcept {
    beat_count_->wait(recieved_count_);
    const uint32_t old = std::exchange(recieved_count_, beat_count_->load());
    return recieved_count_ - old;
  }

private:
  friend class heartbeat;
  explicit heartbeat_waiter(
      std::atomic<uint32_t>* beat_count, uint32_t start_count) noexcept
      : beat_count_{beat_count}, recieved_count_{start_count} {}

private:
  std::atomic<uint32_t>* beat_count_ = nullptr;
  uint32_t recieved_count_ = 0;
};

class heartbeat {
public:
  void beat() {
    ++beat_count_;
    beat_count_.notify_all();
  }

  heartbeat_waiter make_waiter() {
    return heartbeat_waiter{&beat_count_, beat_count_.load()};
  }

private:
  std::atomic<uint32_t> beat_count_ = 0;
};
