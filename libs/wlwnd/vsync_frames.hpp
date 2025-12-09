#pragma once

#include <generator>
#include <optional>
#include <stop_token>

#include <libs/anime/clock.hpp>
#include <libs/wlwnd/wlutil.hpp>

class event_queue;
class event_loop;

struct wl_surface;
struct wl_callback;

class vsync_frames {
public:
  vsync_frames(event_queue& queue, wl_surface& surf, std::stop_token& stop);

  std::generator<frames_clock::time_point> iter();

private:
  event_queue& queue_;
  wl_surface& surf_;
  wl::unique_ptr<wl_callback> frame_cb_;
  std::stop_token& stop_;
};
