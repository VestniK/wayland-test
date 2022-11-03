#pragma once

#include <array>
#include <atomic>
#include <concepts>
#include <optional>

template <std::regular T> class value_update_channel {
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
