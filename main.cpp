#include <array>
#include <iostream>
#include <optional>
#include <sstream>

#include <portable_concurrency/future>

#include "wayland/client.hpp"

using namespace std::literals;

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

  void operator() (wl::registry_ref registry, wl::id id, std::string_view iface, wl::version ver) {
    const bool iface_matched = match_bind(registry, id, iface, ver, std::make_index_sequence<sizeof...(T)>{});

    if (iface_matched && std::count(ids_.begin(), ids_.end(), std::nullopt) == 0)
      promise_.set_value(std::move(ifaces_));
  }

  void operator() (wl::registry_ref, wl::id id) {
    size_t idx = 0;
    std::string_view gone_service;
    ((ids_[idx++] == id ? gone_service = T::interface_name : ""sv), ...);
    if (!gone_service.empty())
      error_.set_exception(std::make_exception_ptr(std::runtime_error{std::string{gone_service} + " is gone"}));
  }

  void operator() (wl::callback_ref, uint32_t) {
    if (std::count(ids_.begin(), ids_.end(), std::nullopt) == 0)
      return;

    size_t idx = 0;
    string_joiner joiner{", "};
    ((joiner + (ids_[idx++] ? T::interface_name : ""sv)), ...);
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
    wl::registry_ref registry, wl::id id, std::string_view iface, wl::version ver,
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

int start(wl::compositor compositor, wl::shell shell) {
  std::cout << "Compositor version: " << compositor.get_version() << '\n';
  std::cout << "Shell version: " << shell.get_version() << '\n';
  wl::surface surface = compositor.create_surface();
  std::cout << "Created a surface of version: " << surface.get_version() << '\n';
  wl::shell_surface shsurf = shell.get_shell_surface(surface);
  std::cout << "Created a shell surface of version: " << shsurf.get_version() << '\n';
  return EXIT_SUCCESS;
}

int main(int argc, char** argv) try {
  wl::display display{argc > 1 ? argv[1] : nullptr};

  registry_searcher<wl::compositor, wl::shell> searcher;

  wl::registry::listener iface_listener = std::ref(searcher);
  wl::registry registry = display.get_registry();
  registry.add_listener(iface_listener);

  wl::callback::listener sync_listener = std::ref(searcher);
  wl::callback sync_cb = display.sync();
  sync_cb.add_listener(sync_listener);

  auto f = searcher.on_found(start);
  while (!f.is_ready())
    display.dispatch();
  return f.get();
} catch (const std::exception& err) {
  std::cerr << "Error: " << err.what() << '\n';
  return  EXIT_FAILURE;
}
