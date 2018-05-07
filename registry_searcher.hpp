#pragma once

#include <array>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

#include <portable_concurrency/future>

#include "wayland/client.hpp"

class string_joiner {
public:
  string_joiner(const std::string& sep): sep_(sep) {}

  string_joiner& operator+ (std::string_view str) {
    if (str.empty())
      return *this;
    if (stream_.tellp() != 0)
      stream_ << sep_;
    stream_ << str;
    return *this;
  }

  operator std::string () const {return stream_.str();}

private:
  std::string sep_;
  std::ostringstream stream_;
};

template<typename... T>
class registry_searcher {
public:
  void operator() (wl::registry::ref registry, wl::id id, std::string_view iface, wl::version ver) {
    const bool iface_matched = match_bind(registry, id, iface, ver, std::make_index_sequence<sizeof...(T)>{});

    if (iface_matched && std::count(ids_.begin(), ids_.end(), std::nullopt) == 0)
      promise_.set_value(std::move(ifaces_));
  }

  void operator() (wl::registry::ref, wl::id id) {
    using std::literals::string_view_literals::operator""sv;
    size_t idx = 0;
    std::string_view gone_service;
    ((ids_[idx++] == id ? gone_service = T::interface_name : ""sv), ...);
    if (!gone_service.empty())
      error_.set_exception(std::make_exception_ptr(std::runtime_error{std::string{gone_service} + " is gone"}));
  }

  void operator() (wl::callback_ref, uint32_t) {
    using std::literals::string_view_literals::operator""sv;
    if (std::count(ids_.begin(), ids_.end(), std::nullopt) == 0)
      return;

    size_t idx = 0;
    string_joiner joiner{", "};
    ((joiner + (!ids_[idx++] ? T::interface_name : ""sv)), ...);
    const std::string missing_services = joiner;

    if (!missing_services.empty())
      error_.set_exception(std::make_exception_ptr(std::runtime_error(missing_services + " not found")));
  }

  template<typename F>
  pc::future<int> on_found(F&& func) {
    pc::future<int> input[] = {
      promise_.get_future().next([func = std::forward<F>(func)](std::tuple<T...> tpl) {return std::apply(func, std::move(tpl));}),
      error_.get_future()
    };
    return pc::when_any(std::begin(input), std::end(input))
      .next([](pc::when_any_result<std::vector<pc::future<int>>> res) {
        return std::move(res.futures[res.index]);
      });
  }

private:
  template<size_t... I>
  bool match_bind(
    wl::registry::ref registry, wl::id id, std::string_view iface, wl::version ver,
    std::index_sequence<I...>
  ) {
    return (
      (iface == T::interface_name ? (std::get<I>(ifaces_) = registry.bind<T>(id, ver), ids_[I] = id, true) : false) || ...
    );
  }

  pc::promise<std::tuple<T...>> promise_;
  pc::promise<int> error_;
  std::tuple<T...> ifaces_;
  std::array<std::optional<wl::id>, sizeof...(T)> ids_;
};
