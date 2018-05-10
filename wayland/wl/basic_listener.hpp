#pragma once

#include "error.hpp"
#include "util.hpp"

namespace wl::detail {

template<typename NativeListener, typename F>
class basic_listener: private F {
public:
  template<typename... A>
  basic_listener(NativeListener listener, A&&... a): F{std::forward<A>(a)...}, native_listener_{listener} {}

  basic_listener(const basic_listener&) = delete;
  basic_listener& operator= (const basic_listener&) = delete;
  basic_listener(basic_listener&&) = delete;
  basic_listener& operator= (basic_listener&&) = delete;

  const NativeListener& native_listener() const {return native_listener_;}

protected:
  F& get_function() {return static_cast<F&>(*this);}

  NativeListener native_listener_;
};

}
