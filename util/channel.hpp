#pragma once

#include <array>
#include <atomic>
#include <cassert>
#include <concepts>
#include <optional>

#if defined(__cpp_lib_hardware_interference_size)
using std::hardware_destructive_interference_size;
#else
inline constexpr size_t hardware_destructive_interference_size = 64;
#endif

template <std::regular T>
class value_update_channel {
private:
  struct box {
    alignas(hardware_destructive_interference_size) T val;
  };

public:
  std::optional<T> get_update() const
      noexcept(std::is_nothrow_copy_constructible_v<T>) {
    const T val = consumer_val();
    if (!try_fetch_update() || val == consumer_val())
      return std::nullopt;
    return consumer_val();
  }
  T get_current() const noexcept(std::is_nothrow_copy_constructible_v<T>&&
          std::is_nothrow_move_constructible_v<T>) {
    const T val = consumer_val();
    if (try_fetch_update())
      return consumer_val();
    return (consumer_ & old_mask) ? val : consumer_val();
  }
  void update(const T& t) noexcept(std::is_nothrow_copy_constructible_v<T>) {
    buf_[producer_].val = t;
    producer_ = cur_.exchange(producer_, std::memory_order::release) &
                ~(seen_mask | old_mask);
  }

private:
  const T& consumer_val() const noexcept {
    return buf_[consumer_ & ~(seen_mask | old_mask)].val;
  }

  bool try_fetch_update() const noexcept {
    // Consumer index is marked as seen in constructor and always marked as seen
    // if new value was fetched from producer. Bit operatrions bellow rely on
    // this invariant.
    assert(consumer_ & seen_mask);
    const auto was_old = consumer_ & old_mask;
    // Producer index is never marked as neither seen nor old. Current index is
    // always marked as seen and may be marked as old.
    //  * Seen but not old index points to an array element with the last value
    //  sent from producer.
    //  * Seen and old index points to an array element whose value was sent
    //  from producer in one of the previous updates and is no more relevant.
    //
    // Before swapping cur_ and consumer_ values  cunsumer_ may be seen or
    // old-seen. After swap cunsumer_ can be new (neither seen nor old-seen) in
    // case if update fetched. In this case cunsumer_ will be marked as seen. If
    // the value sent to cur_ is also seen then we have two "seen not old"
    // indexes and the one stored in the cur_ lies about its "not old" status.
    //
    // Possible states of seen and old bits before and after swap are:
    // s1:o1 -> s1:o0 (no updates prev index pointed to garbage new index points
    //                 to curent value)
    // s1:o1 -> s0:o0 (update, index will be marked as s1:o0 in the end of
    //                 function)
    // s1:o0 -> s1:o1 (no update prev index pointed to current value new index
    //                 points to garbage)
    // s1:o0 -> s0:o0 (update, index will be markes as s1:o0 in the end of
    //                 function, but the value in the cur_ is also s1:o0 and
    //                 next swap can be irrgular)
    // s1:o0 -> s1:o0 (no update and last swap is exatcly the previos case. It
    //                 means that prev index points to current value and new one
    //                 points to garbage. This function must mark cunsumer_ as
    //                 s1:o1)
    //
    // Old bit changes when no update happens:
    // before swap | after swap | in the end |
    //      1      |      0     |    0       |
    //      0      |      1     |    1       |
    //      0      |      0     |    1       | irregular case
    //      1      |      1     |    ?       | impossible case
    // The combination of bit operations bellow changes single bit in "old" bit
    // position for three possible cases and keeps other bits unchanged.
    consumer_ = cur_.exchange(consumer_, std::memory_order::acquire) |
                (was_old ^ old_mask);
    if (consumer_ & seen_mask)
      return false;
    // Update case is handled separatelly here marking index with s1:o0.
    consumer_ = (consumer_ | seen_mask) & ~old_mask;
    return true;
  }

private:
  static constexpr unsigned seen_mask = (1 << 3);
  static constexpr unsigned old_mask = (1 << 4);

private:
  std::array<box, 3> buf_;
  mutable std::atomic<unsigned> cur_ = 0 | old_mask;
  unsigned producer_ = 1;
  mutable unsigned consumer_ = 2 | seen_mask;
};
