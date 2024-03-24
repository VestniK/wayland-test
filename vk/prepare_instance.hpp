#pragma once

#include <functional>

#include <util/clock.hpp>
#include <util/geom.hpp>

struct wl_display;
struct wl_surface;

std::move_only_function<void(frames_clock::time_point)> prepare_instance(
    wl_display& display, wl_surface& surf, size sz);
