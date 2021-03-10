#include <wayland/event_loop.hpp>
#include <wayland/ui_category.hpp>

event_loop::event_loop(gsl::czstring<> display)
    : display_{wl_display_connect(display)} {
  if (!display_)
    throw std::system_error{errno, std::system_category(),
                            "wl_display_connect"};
  registry_.reset(wl_display_get_registry(display_.get()));
  wl_registry_add_listener(registry_.get(), &listener_, this);
  wl_display_roundtrip(display_.get());
  if (const auto ec = check())
    throw std::system_error{ec, "event_loop::event_loop"};
}

std::error_code event_loop::check() noexcept {
  if (!compositor.service)
    return ui_errc::compositor_lost;
  if (!ivi.service)
    return ui_errc::shell_lost;
  return {};
}

void event_loop::dispatch(std::error_code &ec) noexcept {
  if ((ec = check()))
    return;
  wl_display_dispatch(display_.get());
}

void event_loop::dispatch() {
  std::error_code ec;
  dispatch(ec);
  if (ec)
    throw std::system_error{ec, "event_loop::dispatch"};
}

void event_loop::dispatch_pending(std::error_code &ec) noexcept {
  if ((ec = check()))
    return;
  wl_display_dispatch_pending(display_.get());
}

void event_loop::dispatch_pending() {
  std::error_code ec;
  dispatch_pending(ec);
  if (ec)
    throw std::system_error{ec, "event_loop::dispatch_pending"};
}

void event_loop::global(void *data, wl_registry *reg, uint32_t id,
                        const char *name, uint32_t ver) {
  event_loop *self = reinterpret_cast<event_loop *>(data);
  if (name == wl::service_trait<wl_compositor>::name)
    self->compositor = {wl::bind<wl_compositor>(reg, id, ver), id};
  if (name == wl::service_trait<ivi_application>::name)
    self->ivi = {wl::bind<ivi_application>(reg, id, ver), id};
}

void event_loop::global_remove(void *data, wl_registry *, uint32_t id) {
  event_loop *self = reinterpret_cast<event_loop *>(data);
  if (id == self->compositor.id)
    self->compositor = {{}, {}};
  if (id == self->ivi.id)
    self->ivi = {{}, {}};
}
