#include <iostream>

#include "wayland/client.hpp"

struct registry_logger {
  void operator() (wl::registry_ref, wl::id name, std::string_view iface, wl::version ver) {
    std::cout << "wl::registry item added: " << iface << " ver: " << ver << "; id: " << wl::underlying_cast(name) << "\n";
  }

  void operator() (wl::registry_ref, wl::id name) {
    std::cout << "wl::registry item removed: id: " << wl::underlying_cast(name) << "\n";
  }
};

int main(int argc, char** argv) try {
  wl::display display{argc > 1 ? argv[1] : nullptr};

  wl::registry::listener<registry_logger> reg_log;
  wl::registry registry = display.get_registry();
  registry.add_listener(reg_log);
  std::cout << "wl::registry::version: " << registry.get_version() << "\n";

  bool sync_done = false;
  wl::callback::listener sync_done_listener = [&] (wl::callback_ref, uint32_t) {sync_done = true;};
  wl::callback sync_cb = display.sync();
  sync_cb.add_listener(sync_done_listener);

  while (!sync_done)
    display.dispatch();
  return EXIT_SUCCESS;
} catch (const std::exception& err) {
  std::cerr << "Error: " << err.what() << '\n';
  return  EXIT_FAILURE;
}
