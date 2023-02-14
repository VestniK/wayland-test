#include <asio/posix/stream_descriptor.hpp>
#include <asio/use_awaitable.hpp>

#include <wayland/event_loop.hpp>
#include <wayland/ui_category.hpp>

event_loop::event_loop(const char *display)
    : display_{wl_display_connect(display)} {
  if (!display_)
    throw std::system_error{errno, std::system_category(),
                            "wl_display_connect"};
}

registry::registry(event_loop &eloop) {
  registry_.reset(wl_display_get_registry(&eloop.get_display()));
  wl_registry_add_listener(registry_.get(), &listener_, this);
  wl_display_roundtrip(&eloop.get_display());
  if (const auto ec = check())
    throw std::system_error{ec, "event_loop::event_loop"};
}

std::error_code registry::check() noexcept {
  if (!compositor_.service)
    return ui_errc::compositor_lost;
  if (!ivi_.service && !xdg_wm_.service)
    return ui_errc::shell_lost;
  return {};
}

void event_loop::dispatch() noexcept { wl_display_dispatch(display_.get()); }

void event_loop::dispatch_pending() noexcept {
  wl_display_dispatch_pending(display_.get());
}

asio::awaitable<void>
event_loop::dispatch_once(asio::io_context::executor_type exec) {
  if (wl_display_prepare_read(&get_display()) != 0) {
    dispatch_pending();
    co_return;
  }
  wl_display_flush(&get_display());
  asio::posix::stream_descriptor conn{exec, wl_display_get_fd(&get_display())};
  co_await conn.async_wait(asio::posix::stream_descriptor::wait_read,
                           asio::use_awaitable);
  conn.release();
  wl_display_read_events(&get_display());
  notify_queues();
  // TODO `wl_display_cancel_read(display);` on wait failure!!!
  dispatch_pending();
  co_return;
}

void registry::global(void *data, wl_registry *reg, uint32_t id,
                      const char *name, uint32_t ver) {
  registry *self = reinterpret_cast<registry *>(data);
  if (name == wl::service_trait<wl_compositor>::name)
    self->compositor_ = {wl::bind<wl_compositor>(reg, id, ver), id};
  if (name == wl::service_trait<ivi_application>::name)
    self->ivi_ = {wl::bind<ivi_application>(reg, id, ver), id};
  if (name == wl::service_trait<xdg_wm_base>::name)
    self->xdg_wm_ = {wl::bind<xdg_wm_base>(reg, id, ver), id};
}

void registry::global_remove(void *data, wl_registry *, uint32_t id) {
  registry *self = reinterpret_cast<registry *>(data);
  if (id == self->compositor_.id)
    self->compositor_ = {{}, {}};
  if (id == self->ivi_.id)
    self->ivi_ = {{}, {}};
  if (id == self->xdg_wm_.id)
    self->xdg_wm_ = {{}, {}};
}
