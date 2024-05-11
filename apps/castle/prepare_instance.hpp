#pragma once

#include <memory>

#include <libs/anime/clock.hpp>
#include <libs/geom/geom.hpp>

struct wl_display;
struct wl_surface;

struct renderer_iface {
  virtual void resize(size sz) = 0;
  virtual void draw(frames_clock::time_point ts) = 0;
  virtual ~renderer_iface() noexcept = default;
};

std::unique_ptr<renderer_iface> prepare_instance(
    wl_display& display, wl_surface& surf, size sz);
