#include <system_error>

#include <xdg-shell.h>

#include <wayland/gui_shell.hpp>
#include <wayland/ui_category.hpp>

namespace wl {

namespace {

void xdg_ping(void*, xdg_wm_base* wm, uint32_t serial) {
  xdg_wm_base_pong(wm, serial);
}

constexpr uint32_t ivi_main_glapp_id = 1337;

} // namespace

gui_shell::gui_shell(event_loop& eloop) : xdg_listener_{.ping = &xdg_ping} {
  registry_.reset(wl_display_get_registry(&eloop.get_display()));
  wl_registry_add_listener(registry_.get(), &listener_, this);
  wl_display_roundtrip(&eloop.get_display());
  if (ivi_.service)
    xdg_wm_ = {{}, {}};

  if (xdg_wm_.service)
    xdg_wm_base_add_listener(xdg_wm_.service.get(), &xdg_listener_, nullptr);

  if (const auto ec = check())
    throw std::system_error{ec, "gui_shell::gui_shell"};
}

std::error_code gui_shell::check() noexcept {
  if (!compositor_.service)
    return ui_errc::compositor_lost;
  if (!ivi_.service && !xdg_wm_.service)
    return ui_errc::shell_lost;
  return {};
}

asio::awaitable<sized_window<shell_window>> gui_shell::create_maximized_window(
    event_loop& eloop, asio::io_context::executor_type io_exec) {
  struct : xdg::delegate {
    void resize(size sz) override { wnd_size = sz; }
    void close() override { closed = true; }

    std::optional<size> wnd_size;
    bool closed = false;
  } szdelegate;

  shell_window wnd;
  if (get_ivi())
    wnd = shell_window{ivi::window{
        *get_compositor(), *get_ivi(), ivi_main_glapp_id, &szdelegate}};
  else {
    xdg::toplevel_window xdg_wnd{*get_compositor(), *get_xdg_wm(), &szdelegate};
    xdg_wnd.maximize();
    wnd = shell_window{std::move(xdg_wnd)};
  }
  while (!szdelegate.wnd_size && !szdelegate.closed)
    co_await eloop.dispatch_once(io_exec);

  if (szdelegate.closed)
    throw std::system_error{ui_errc::window_closed, "create_maximized_window"};

  co_return sized_window<shell_window>{
      .window = std::move(wnd), .sz = szdelegate.wnd_size.value()};
}

void gui_shell::global(
    void* data, wl_registry* reg, uint32_t id, const char* name, uint32_t ver) {
  gui_shell* self = reinterpret_cast<gui_shell*>(data);
  if (name == wl::service_trait<wl_compositor>::name)
    self->compositor_ = {wl::bind<wl_compositor>(reg, id, ver), id};
  if (name == wl::service_trait<ivi_application>::name)
    self->ivi_ = {wl::bind<ivi_application>(reg, id, ver), id};
  if (name == wl::service_trait<xdg_wm_base>::name)
    self->xdg_wm_ = {wl::bind<xdg_wm_base>(reg, id, ver), id};
}

void gui_shell::global_remove(void* data, wl_registry*, uint32_t id) {
  gui_shell* self = reinterpret_cast<gui_shell*>(data);
  if (id == self->compositor_.id)
    self->compositor_ = {{}, {}};
  if (id == self->ivi_.id)
    self->ivi_ = {{}, {}};
  if (id == self->xdg_wm_.id)
    self->xdg_wm_ = {{}, {}};
}

static_assert(window<xdg::toplevel_window>);
static_assert(window<ivi::window>);

} // namespace wl
