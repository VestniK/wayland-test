#include <libs/wlwnd/event_loop.hpp>
#include <libs/wlwnd/vsync_frames.hpp>

vsync_frames::vsync_frames(
    event_queue& queue, wl_surface& surf, std::stop_token& stop)
    : queue_{queue}, surf_{surf}, frame_cb_{wl_surface_frame(&surf)},
      stop_{stop} {}

std::optional<vsync_frames::value_type> vsync_frames::wait() {
  std::optional<uint32_t> next_frame;
  wl_callback_listener listener = {
      .done = [](void* data, wl_callback*, uint32_t ts) {
        *reinterpret_cast<std::optional<uint32_t>*>(data) = ts;
      }};
  wl_callback_add_listener(frame_cb_.get(), &listener, &next_frame);
  while (!next_frame) {
    if (stop_.stop_requested())
      return std::nullopt;
    queue_.dispatch();
  }
  frame_cb_ = wl::unique_ptr<wl_callback>{wl_surface_frame(&surf_)};
  return value_type{frames_clock::duration{next_frame.value()}};
}

static_assert(std::input_iterator<vsync_frames::iterator>);
static_assert(
    std::sentinel_for<vsync_frames::sentinel, vsync_frames::iterator>);
