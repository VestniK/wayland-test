#include <iostream>
#include <map>

#include <gsl/string_span>

#include <util/util.hpp>
#include <wayland/client.hpp>

struct ouptut_listener {
  void geometry(
    wl::output::ref, wl::point, wl::physical_size size, wl::output::subpixel, gsl::czstring<> make,
    gsl::czstring<> name, wl::output::transform
  ) {
    std::cout
      << "\tMonitor: " << make << "[" << name << "]" << " " << ut::underlying_cast(size.width)
      << "x" << ut::underlying_cast(size.height) << "mm\n";
  }
  void mode(wl::output::ref, wl::output::mode_flags, wl::size sz, int refresh) {
    std::cout << "\t\tmode: " << sz.width << 'x' << sz.height << '@' << refresh << "mHz\n";
  }
  void done(wl::output::ref) {}
  void scale(wl::output::ref, int) {}
};

struct registry_logger {
  void global(wl::registry::ref reg, wl::id id, std::string_view iface, wl::version ver) {
    std::cout << "wl::registry item added: " << iface << " ver: " << ver << "; id: " << ut::underlying_cast(id) << "\n";
    if (iface == wl::output::interface_name)
      outputs.emplace(id, reg.bind<wl::output>(id, ver));
  }

  void global_remove(wl::registry::ref, wl::id id) {
    std::cout << "wl::registry item removed: id: " << ut::underlying_cast(id) << "\n";
    if (auto it = outputs.find(id); it != outputs.end())
      outputs.erase(it);
  }

  std::map<wl::id, wl::output> outputs;
};

int main(int argc, char** argv) try {
  wl::display display{argc > 1 ? argv[1] : nullptr};

  registry_logger reg_log;
  wl::registry registry = display.get_registry();
  registry.add_listener(reg_log);
  std::cout << "wl::registry::version: " << registry.get_version() << "\n";

  bool sync_done = false;
  auto sync_done_listener = [&] (wl::callback::ref, uint32_t) {sync_done = true;};
  wl::callback sync_cb = display.sync();
  sync_cb.add_listener(sync_done_listener);

  while (!sync_done)
    display.dispatch();

  ouptut_listener out_log;
  for (auto&[id, out]: reg_log.outputs) {
    sync_done = false;
    out.add_listener(out_log);
    sync_cb = display.sync();
    sync_cb.add_listener(sync_done_listener);
    while (!sync_done)
      display.dispatch();
  }

  return EXIT_SUCCESS;
} catch (const std::exception& err) {
  std::cerr << "Error: " << err.what() << '\n';
  return  EXIT_FAILURE;
}
