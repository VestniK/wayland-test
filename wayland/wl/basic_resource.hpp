#pragma once

#include "error.hpp"
#include "util.hpp"

namespace wl::detail {

template<typename Resource, template<typename> class Iface>
class basic_resource: public Iface<basic_resource<Resource, Iface>> {
public:
  using resource_type = Resource;
  using native_handle_type = Resource*;
  native_handle_type native_handle() const {return ptr_.get();}

  basic_resource() noexcept = default;
  basic_resource(unique_ptr<Resource> resource) noexcept: ptr_(std::move(resource)) {}

  explicit operator bool () const noexcept {return static_cast<bool>(ptr_);}

  class ref: public Iface<ref> {
  public:
    using native_handle_type = Resource*;
    native_handle_type native_handle() const {return &ref_.get();}

    ref(Resource& resource) noexcept: ref_(resource) {}

  private:
    std::reference_wrapper<Resource> ref_;
  };

private:
  unique_ptr<Resource> ptr_;
};

}
