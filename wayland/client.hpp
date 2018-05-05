#pragma once

#include <string>
#include <string_view>
#include <system_error>

#include <wayland-client.h>

#include "wl/callback.hpp"
#include "wl/error.hpp"
#include "wl/registry.hpp"
#include "wl/shm.hpp"
#include "wl/util.hpp"

namespace wl {

class surface {
public:
  using native_handle_type = wl_surface*;
  native_handle_type native_handle() const {return ptr_.get();}

  surface() noexcept = default;
  surface(unique_ptr<wl_surface> resource) noexcept: ptr_(std::move(resource)) {}

  version get_version() const noexcept {return version{wl_surface_get_version(ptr_.get())};}
private:
  unique_ptr<wl_surface> ptr_;
};

class compositor {
public:
  using resource_type = wl_compositor;
  static const wl_interface& resource_interface;
  static constexpr std::string_view interface_name = "wl_compositor"sv;

  compositor() noexcept = default;
  compositor(unique_ptr<resource_type> resource) noexcept: ptr_(std::move(resource)) {}

  explicit operator bool () const noexcept {return static_cast<bool>(ptr_);}
  version get_version() const noexcept {return version{wl_compositor_get_version(ptr_.get())};}

  surface create_surface() {return unique_ptr<wl_surface>{wl_compositor_create_surface(ptr_.get())};}

private:
  unique_ptr<resource_type> ptr_;
};
const wl_interface& compositor::resource_interface = wl_compositor_interface;

class shell_surface {
public:
  using native_handle_type = wl_shell_surface*;
  native_handle_type native_handle() const {return ptr_.get();}

  shell_surface() noexcept = default;
  shell_surface(unique_ptr<wl_shell_surface> resource): ptr_(std::move(resource)) {}

  version get_version() const noexcept {return version{wl_shell_surface_get_version(ptr_.get())};}

  void set_toplevel() const {wl_shell_surface_set_toplevel(ptr_.get());}

private:
  unique_ptr<wl_shell_surface> ptr_;
};

class shell {
public:
  using resource_type = wl_shell;
  static const wl_interface& resource_interface;
  static constexpr std::string_view interface_name = "wl_shell"sv;

  shell() noexcept = default;
  shell(unique_ptr<resource_type> resource) noexcept: ptr_(std::move(resource)) {}

  explicit operator bool () const noexcept {return static_cast<bool>(ptr_);}

  version get_version() const noexcept {return version{wl_shell_get_version(ptr_.get())};}

  shell_surface get_shell_surface(const surface& surf) {
    return {unique_ptr<wl_shell_surface>{wl_shell_get_shell_surface(ptr_.get(), surf.native_handle())}};
  }

private:
  unique_ptr<resource_type> ptr_;
};
const wl_interface& shell::resource_interface = wl_shell_interface;

class display {
public:
  explicit display(const char* name = nullptr): ptr_(wl_display_connect(name)) {
    using std::literals::operator""s;
    if (!ptr_)
      throw std::system_error{errno, std::system_category(), "wl::display::connect("s + (name ? name : "") + ")"};
  }

  registry get_registry() const {
    return registry{unique_ptr<wl_registry>{wl_display_get_registry(ptr_.get())}};
  }

  callback sync() {return unique_ptr<wl_callback>{wl_display_sync(ptr_.get())};}

  void dispatch() const {
    if (wl_display_dispatch(ptr_.get()) < 0)
      throw std::runtime_error("wl::display::dispatch failed");
  }

private:
  unique_ptr<wl_display> ptr_;
};

} // namespace wl
