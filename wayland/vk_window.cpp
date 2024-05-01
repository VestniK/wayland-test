#include "vk_window.hpp"

#include <spdlog/spdlog.h>

#include <libs/sync/channel.hpp>
#include <libs/sync/task_guard.hpp>

#include <vk/prepare_instance.hpp>

#include <wayland/event_loop.hpp>
#include <wayland/xdg_window.hpp>

struct vk_window::impl : public xdg::delegate {
  impl(co::pool_executor exec, event_queue& queue, wl_surface& surf,
      size initial_size)
      : render_task_guard{
            exec, [&queue, &surf, initial_size](std::stop_token stop) mutable {
              std::stop_callback wake_queue_on_stop{
                  stop, [&queue] { queue.wake(); }};
              vsync_frames frames{queue, surf, stop};
              auto render =
                  prepare_instance(queue.display(), surf, initial_size);
              render({});
              for (auto ts : frames) {
                render(ts);
              }
            }} {}

  void close() override {
    spdlog::debug("Window close event received");
    render_task_guard.stop();
  }
  void resize(size sz) override { resize_channel.update(sz); }

  value_update_channel<size> resize_channel;
  task_guard render_task_guard;
};

vk_window::vk_window(event_queue queue, co::pool_executor pool_exec,
    wl::sized_window<wl::shell_window>&& wnd)
    : wnd_{std::move(wnd.window)}, queue_{std::move(queue)} {
  wl_surface& surf = wnd_.get_surface();
  wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(&surf), &queue.get());
  impl_ = std::make_unique<impl>(pool_exec, queue_, surf, wnd.sz);
  wnd_.set_delegate(impl_.get());
}

vk_window::vk_window(vk_window&&) noexcept = default;
vk_window& vk_window::operator=(vk_window&&) noexcept = default;

vk_window::~vk_window() noexcept = default;

[[nodiscard]] bool vk_window::is_closed() const noexcept {
  return !impl_ || impl_->render_task_guard.is_finished() ||
         impl_->render_task_guard.is_finishing();
}
