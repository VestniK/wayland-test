#pragma once

#include <chrono>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <util/channel.hpp>
#include <util/clock.hpp>

#include <gamepad/types/axis_state.hpp>
#include <gamepad/types/kyes.hpp>

struct animate_to {
  glm::vec2 dest;
  std::chrono::milliseconds duration;

  bool operator==(const animate_to&) const noexcept = default;
};
struct linear_animation {
  glm::vec2 start;
  frames_clock::time_point start_time;
  glm::vec2 end;
  frames_clock::time_point end_time;

  void reset(frames_clock::time_point now, animate_to animation) noexcept {
    start = animate(now);
    start_time = now;
    end = animation.dest;
    end_time = now + animation.duration;
  }

  glm::vec2 animate(frames_clock::time_point now) const noexcept {
    if (now >= end_time)
      return end;
    const auto factor = float_time::milliseconds{now - start_time} /
                        float_time::milliseconds{end_time - start_time};
    return start + (end - start) * factor;
  }
};

namespace scene {

class controller {
public:
  controller();

  void operator()(gamepad::key key, bool pressed);
  void operator()(gamepad::axis axis, gamepad::axis2d_state state);
  void operator()(gamepad::axis axis, gamepad::axis3d_state state);

  glm::vec2 current_cube_vel() const noexcept {
    return cube_vel_.get_current();
  }

  std::optional<animate_to> cube_tex_offset_update() const noexcept {
    return cube_tex_offset_update_.get_update();
  }

private:
  value_update_channel<animate_to> cube_tex_offset_update_;
  value_update_channel<glm::vec2> cube_vel_;
};

} // namespace scene
