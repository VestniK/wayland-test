#include <libs/wlwnd/event_loop.hpp>
#include <libs/wlwnd/vsync_frames.hpp>

vsync_frames::vsync_frames(event_queue& queue, wl_surface& surf, std::stop_token& stop)
    : queue_{queue}, surf_{surf}, frame_cb_{wl_surface_frame(&surf)}, stop_{stop} {}

std::generator<frames_clock::time_point> vsync_frames::iter() {
  while (!stop_.stop_requested()) {
    std::optional<uint32_t> next_frame;
    wl_callback_listener listener = {.done = [](void* data, wl_callback*, uint32_t ts) {
      *reinterpret_cast<std::optional<uint32_t>*>(data) = ts;
    }};
    wl_callback_add_listener(frame_cb_.get(), &listener, &next_frame);
    while (!next_frame) {
      if (stop_.stop_requested())
        co_return;
      queue_.dispatch();
    }
    frame_cb_ = wl::unique_ptr<wl_callback>{wl_surface_frame(&surf_)};
    co_yield frames_clock::time_point{frames_clock::duration{next_frame.value()}};
  }
}
