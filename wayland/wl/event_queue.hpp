#pragma once

#include "util.hpp"

namespace wl {

class event_queue {
public:
  using native_handle_type = wl_event_queue*;

  event_queue() noexcept = default;
  event_queue(unique_ptr<wl_event_queue> ptr): ptr_{std::move(ptr)} {}

  native_handle_type native_handle() noexcept {return ptr_.get();}

private:
  unique_ptr<wl_event_queue> ptr_;
};

}
