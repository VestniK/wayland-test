#pragma once

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
  struct iterator;
  struct sentinel {};
  using value_type = frames_clock::time_point;

  vsync_frames(event_queue& queue, wl_surface& surf, std::stop_token& stop);

  iterator begin();
  sentinel end() const { return {}; }

private:
  std::optional<value_type> wait();

private:
  event_queue& queue_;
  wl_surface& surf_;
  wl::unique_ptr<wl_callback> frame_cb_;
  std::stop_token& stop_;
};

struct vsync_frames::iterator {
  using value_type = vsync_frames::value_type;
  using iterator_category = std::input_iterator_tag;
  using reference = const value_type&;
  using pointer = const value_type*;
  using difference_type = std::ptrdiff_t;

  reference operator*() const noexcept { return current; }
  iterator& operator++() {
    if (auto next = vsync->wait())
      current = next.value();
    else
      vsync = nullptr;

    return *this;
  }
  iterator operator++(int) {
    auto res = *this;
    ++(*this);
    return res;
  }

  vsync_frames* vsync = nullptr;
  value_type current;
};

inline vsync_frames::iterator vsync_frames::begin() {
  if (auto first = wait())
    return iterator{.vsync = this, .current = first.value()};
  else
    return iterator{.vsync = nullptr, .current = {}};
}

inline constexpr bool operator==(const vsync_frames::iterator& it, vsync_frames::sentinel) noexcept {
  return it.vsync == nullptr;
}
