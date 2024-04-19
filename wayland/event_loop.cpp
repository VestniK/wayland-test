#include <asio/posix/stream_descriptor.hpp>
#include <asio/use_awaitable.hpp>

#include <wayland/event_loop.hpp>
#include <wayland/ui_category.hpp>

event_loop::event_loop(const char* display)
    : display_{wl_display_connect(display)} {
  if (!display_)
    throw std::system_error{
        errno, std::system_category(), "wl_display_connect"};
}

void event_loop::dispatch() noexcept { wl_display_dispatch(display_.get()); }

void event_loop::dispatch_pending() noexcept {
  wl_display_dispatch_pending(display_.get());
}

asio::awaitable<void> event_loop::dispatch_once(
    asio::io_context::executor_type exec) {
  if (wl_display_prepare_read(&get_display()) != 0) {
    dispatch_pending();
    co_return;
  }
  wl_display_flush(&get_display());
  asio::posix::stream_descriptor conn{exec, wl_display_get_fd(&get_display())};
  co_await conn.async_wait(
      asio::posix::stream_descriptor::wait_read, asio::use_awaitable);
  conn.release();
  wl_display_read_events(&get_display());
  heartbeat_.beat();
  // TODO `wl_display_cancel_read(display);` on wait failure!!!
  dispatch_pending();
  co_return;
}
