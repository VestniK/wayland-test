#pragma once

#include <functional>

#include <libs/anime/clock.hpp>
#include <libs/geom/geom.hpp>

struct wl_display;
struct wl_surface;
class vsync_frames;
template <std::regular T>
class value_update_channel;

using animation_function = std::move_only_function<void(
    wl_display&, wl_surface&, vsync_frames&, value_update_channel<size>&)>;

template <typename T>
concept renderer = requires(T& t, size sz, frames_clock::time_point tp) {
  { t.resize(sz) };
  { t.draw(tp) };
};
