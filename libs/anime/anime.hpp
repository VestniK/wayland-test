#pragma once

#include <chrono>
#include <utility>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <libs/anime/clock.hpp>

struct animate_to {
  glm::vec3 dest;
  std::chrono::milliseconds duration;

  bool operator==(const animate_to&) const noexcept = default;
};
struct linear_animation {
  glm::vec3 start;
  frames_clock::time_point start_time;
  glm::vec3 end;
  frames_clock::time_point end_time;

  void reset(frames_clock::time_point now, animate_to animation) noexcept {
    start = animate(now);
    start_time = now;
    end = animation.dest;
    end_time = now + animation.duration;
  }

  glm::vec3 animate(frames_clock::time_point now) const noexcept {
    if (now >= end_time)
      return end;
    const auto factor =
        float_time::milliseconds{now - start_time} / float_time::milliseconds{end_time - start_time};
    return start + (end - start) * factor;
  }
};

struct bounding_box {
  glm::vec2 min = glm::vec2{
      std::numeric_limits<glm::vec2::value_type>::min(), std::numeric_limits<glm::vec2::value_type>::min()
  };
  glm::vec2 max = glm::vec2{
      std::numeric_limits<glm::vec2::value_type>::max(), std::numeric_limits<glm::vec2::value_type>::max()
  };
  ;

  glm::vec2 clamp(glm::vec2 pt) const noexcept {
    return {std::clamp(pt.x, min.x, max.x), std::clamp(pt.y, min.y, max.y)};
  }
};

class clamped_integrator {
public:
  clamped_integrator(bounding_box bounds, glm::vec2 start_val, frames_clock::time_point start_time) noexcept
      : bounds_{bounds}, current_{start_val}, last_ts_{start_time} {}

  glm::vec2 operator()(glm::vec2 velocity, frames_clock::time_point ts) noexcept {
    current_ +=
        velocity * std::chrono::duration_cast<float_time::seconds>(ts - std::exchange(last_ts_, ts)).count();
    current_ = bounds_.clamp(current_);
    return current_;
  }

private:
  bounding_box bounds_;
  glm::vec2 current_;
  frames_clock::time_point last_ts_;
};
