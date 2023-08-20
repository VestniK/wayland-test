#pragma once

#include <chrono>

struct frames_clock {
  using duration = std::chrono::milliseconds;
  using rep = duration::rep;
  using period = duration::period;
  using time_point = std::chrono::time_point<frames_clock, duration>;

  static constexpr bool is_steady = true;
};

namespace float_time {

using seconds = std::chrono::duration<float>;
using milliseconds = std::chrono::duration<float, std::milli>;

} // namespace float_time
