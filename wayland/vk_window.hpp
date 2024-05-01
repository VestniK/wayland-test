#pragma once

#include <functional>
#include <memory>

#include <libs/sync/channel.hpp>

#include <wayland/event_loop.hpp>
#include <wayland/shell_window.hpp>
#include <wayland/vsync_frames.hpp>

#include <corort/executors.hpp>

class vk_window {
public:
  vk_window() noexcept = default;
  vk_window(event_queue queue, co::pool_executor pool_exec,
      wl::sized_window<wl::shell_window>&& wnd);

  vk_window(const vk_window&) = delete;
  vk_window& operator=(const vk_window&) = delete;

  vk_window(vk_window&&) noexcept;
  vk_window& operator=(vk_window&&) noexcept;

  ~vk_window() noexcept;

  [[nodiscard]] bool is_closed() const noexcept;

private:
  struct impl;

private:
  wl::shell_window wnd_;
  event_queue queue_;
  std::unique_ptr<impl> impl_;
};
