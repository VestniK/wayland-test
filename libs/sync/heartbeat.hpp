#pragma once

#include <atomic>
#include <cstdint>
#include <utility>

class heartbeat;

class heartbeat_waiter {
private:
  constexpr static uint32_t beat_mask = 0x00'ff'ff'ff;
  constexpr static uint32_t wake_step = 0x01'00'00'00;

public:
  constexpr heartbeat_waiter() noexcept = default;

  uint32_t wait_new_beats() noexcept {
    beat_count_->wait(recieved_count_);
    const uint32_t old = std::exchange(recieved_count_, beat_count_->load());
    return (recieved_count_ & beat_mask) - (old & beat_mask);
  }

  void wake() const noexcept { wake_impl(*beat_count_); }

private:
  friend class heartbeat;
  explicit heartbeat_waiter(
      std::atomic<uint32_t>* beat_count, uint32_t start_count) noexcept
      : beat_count_{beat_count}, recieved_count_{start_count} {}

  static void wake_impl(std::atomic<uint32_t>& beat_count) noexcept {
    beat_count.fetch_add(wake_step);
    beat_count.notify_all();
  }

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

  void wake_waiters() noexcept { heartbeat_waiter::wake_impl(beat_count_); }

  heartbeat_waiter make_waiter() {
    return heartbeat_waiter{&beat_count_, beat_count_.load()};
  }

private:
  std::atomic<uint32_t> beat_count_ = 0;
};
