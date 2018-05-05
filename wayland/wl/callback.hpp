#pragma once

#include <wayland-client.h>

#include "error.hpp"
#include "util.hpp"

namespace wl {

template<typename Callback>
struct callback_interface {
  version get_version() const {
    return version{wl_callback_get_version(static_cast<const Callback&>(*this).native_handle())};
  }
};

class callback_ref: public callback_interface<callback_ref> {
public:
  using native_handle_type = wl_callback*;
  callback_ref(native_handle_type handle): ref_(*handle) {}

  native_handle_type native_handle() const {return &ref_.get();}

private:
  std::reference_wrapper<wl_callback> ref_;
};

class callback: public callback_interface<callback> {
public:
  using native_handle_type = wl_callback*;
  native_handle_type native_handle() const noexcept {return ptr_.get();}

  callback(unique_ptr<wl_callback> ptr): ptr_(std::move(ptr)) {}

  version get_version() const noexcept {return version{wl_callback_get_version(ptr_.get())};}

  template<typename F>
  class listener: private F {
  public:
    listener(): F() {}
    template<typename... A>
    listener(A&&... a): F(std::forward<F>(a)...) {}

    listener(const listener&) = delete;
    listener& operator= (const listener&) = delete;
    listener(listener&&) = delete;
    listener& operator= (listener&&) = delete;

  private:
    F& get_function() noexcept {return static_cast<F&>(*this);}

    static void done(void* data, wl_callback* cb, uint32_t cb_data) {
      auto self = static_cast<listener *>(data);
      std::invoke(self->get_function(), callback_ref{cb}, cb_data);
    }

    friend class wl::callback;

    wl_callback_listener listener_ = {&done};
  };
  template<typename F> listener(F&&) -> listener<std::decay_t<F>>;

  template<typename F>
  void add_listener(listener<F>& l) {
    if (wl_callback_add_listener(ptr_.get(), &l.listener_, &l) < 0)
      throw std::system_error{errc::add_callback_listener_failed};
  }

private:
  unique_ptr<wl_callback> ptr_;
};

}
