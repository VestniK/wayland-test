#pragma once

#include <util/geom.hpp>

struct wl_display;
struct wl_surface;

void prepare_instance(wl_display& display, wl_surface& surf, size sz);
