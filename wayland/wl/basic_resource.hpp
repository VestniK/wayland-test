#pragma once

#include "error.hpp"
#include "util.hpp"

namespace wl::detail {

template<typename Resource, template<typename> class Iface>
class basic_ref: public Iface<basic_ref<Resource, Iface>> {
public:
  using native_handle_type = Resource*;
  native_handle_type native_handle() const {return &ref_.get();}

  basic_ref(Resource& resource) noexcept: ref_(resource) {}

  version get_version() const {
    return version{wl_proxy_get_version(reinterpret_cast<wl_proxy*>(native_handle()))};
  }

private:
  std::reference_wrapper<Resource> ref_;
};

template<typename Resource, template<typename> class Iface>
class basic_resource: public Iface<basic_resource<Resource, Iface>> {
public:
  using resource_type = Resource;
  using ref = basic_ref<Resource, Iface>;
  using native_handle_type = Resource*;
  native_handle_type native_handle() const {return ptr_.get();}

  basic_resource() noexcept = default;
  basic_resource(unique_ptr<Resource> resource) noexcept: ptr_(std::move(resource)) {}

  explicit operator bool () const noexcept {return static_cast<bool>(ptr_);}

  operator ref () {
    if (!ptr_)
      throw std::runtime_error{"Attempt to get refference to empty resource"};
    return ref{*ptr_.get()};
  }

  version get_version() const {
    return version{wl_proxy_get_version(reinterpret_cast<wl_proxy*>(native_handle()))};
  }

private:
  unique_ptr<Resource> ptr_;
};

template<typename T>
struct resource_ref;

template<typename Resource, template<typename> class Iface>
struct resource_ref<basic_resource<Resource, Iface>> {using type = typename basic_resource<Resource, Iface>::ref;};

template<typename Resource, template<typename> class Iface>
struct resource_ref<basic_ref<Resource, Iface>> {
  using type = basic_ref<Resource, Iface>;
};

template<typename T>
using resource_ref_t = typename resource_ref<T>::type;

template<typename Resource, typename T>
auto native_handle(const T& t) {return static_cast<const Resource&>(t).native_handle();}

}
