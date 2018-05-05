#pragma once

#include <string_view>

#include "error.hpp"
#include "util.hpp"

namespace wl {

template<typename F>
class shm_listener: public F {
public:
  shm_listener(): F() {}
  template<typename... A>
  shm_listener(A&&... a): F(std::forward<F>(a)...) {}

  shm_listener(const shm_listener&) = delete;
  shm_listener& operator= (const shm_listener&) = delete;
  shm_listener(shm_listener&&) = delete;
  shm_listener& operator= (shm_listener&&) = delete;

  operator const wl_shm_listener& () const noexcept {return listener_;}

private:
  F& get_function() {return static_cast<F&>(*this);}

  static void format(void* data, wl_shm* handle, uint32_t fmt);

  wl_shm_listener listener_ = {&format};
};
template<typename F> shm_listener(F&&) -> shm_listener<std::decay_t<F>>;

template<typename SHM>
struct shm_interface {
  version get_version() const noexcept {
    return version{wl_shm_get_version(static_cast<const SHM&>(*this).native_handle())};
  }

  template<typename F>
  void add_listener(shm_listener<F>& listener) {
    wl_shm* shm = static_cast<SHM&>(*this).native_handle();
    const wl_shm_listener& native_listener = listener;
    if (::wl_shm_add_listener(shm, &native_listener, &listener) != 0)
      throw std::system_error{errc::add_shm_listener_failed};
  }
};

class shm: public shm_interface<shm> {
public:
  using native_handle_type = wl_shm*;
  native_handle_type native_handle() const {return ptr_.get();}

  using resource_type = wl_shm;
  static const wl_interface& resource_interface;
  static constexpr std::string_view interface_name = "wl_shm"sv;

  template<typename F>
  using listener = shm_listener<F>;

  shm() noexcept = default;
  shm(unique_ptr<resource_type> resource) noexcept: ptr_(std::move(resource)) {}

  explicit operator bool () const noexcept {return static_cast<bool>(ptr_);}

private:
  unique_ptr<resource_type> ptr_;
};
const wl_interface& shm::resource_interface = wl_shm_interface;

class shm_ref: public shm_interface<shm> {
public:
  using native_handle_type = wl_shm*;
  native_handle_type native_handle() const {return &ref_.get();}

  shm_ref(wl_shm& resource) noexcept: ref_(resource) {}

private:
  std::reference_wrapper<wl_shm> ref_;
};

template<typename F>
void shm_listener<F>::format(void* data, wl_shm* handle, uint32_t fmt) {
  shm_listener* self = static_cast<shm_listener*>(data);
  std::invoke(self->get_function(), shm_ref{*handle}, fmt);
}

};
