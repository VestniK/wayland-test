#include <memory>
#include <system_error>

#include <libudev.h>

#include <spdlog/spdlog.h>

namespace ludev {

struct deleter {
  void operator()(udev *p) const noexcept { udev_unref(p); }
  void operator()(udev_enumerate *p) const noexcept { udev_enumerate_unref(p); }
  void operator()(udev_device *p) const noexcept { udev_device_unref(p); }
};

template <typename T> using unique_ptr = std::unique_ptr<T, deleter>;

void list_gamepads() {
  unique_ptr<udev> udev{udev_new()};
  unique_ptr<udev_enumerate> enumerator{udev_enumerate_new(udev.get())};
  udev_enumerate_add_match_subsystem(enumerator.get(), "input");
  udev_enumerate_add_match_property(enumerator.get(), "ID_INPUT_JOYSTICK", "1");
  if (int ret = udev_enumerate_scan_devices(enumerator.get()); ret < 0)
    throw std::system_error{-ret, std::system_category(),
                            "udev_enumerate_scan_devices"};
  udev_list_entry *entry = nullptr;
  udev_list_entry_foreach(entry,
                          udev_enumerate_get_list_entry(enumerator.get())) {
    const char *syspath = udev_list_entry_get_name(entry);
    unique_ptr<udev_device> dev{
        udev_device_new_from_syspath(udev.get(), syspath)};
    const char *devnode = udev_device_get_devnode(dev.get());
    spdlog::info("found gamepad:\n\t{}\n\t{}", syspath,
                 devnode ? devnode : "nullptr");
  }
}

} // namespace ludev
