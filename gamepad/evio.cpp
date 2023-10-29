#include "evio.hpp"

#include <fmt/format.h>

namespace evio {

input_absinfo load_absinfo(asio::posix::stream_descriptor& dev, evio::axe axe) {
  input_absinfo axis_limits{};
  if (ioctl(dev.native_handle(), EVIOCGABS(axe.code), &axis_limits) == -1)
    throw std::system_error{errno, std::system_category(),
        fmt::format("ioctl(EVIOCGNAME({}))", axe.define_name)};
  return axis_limits;
}

std::expected<input_absinfo, evio::error> load_absinfo(
    asio::posix::stream_descriptor& dev, gamepad::axis axis,
    gamepad::dimention coord) {
  return evio::axis2evcode(axis, coord).transform([&](auto axe) {
    return load_absinfo(dev, axe);
  });
}

input_id load_dev_id(asio::posix::stream_descriptor& dev) {
  input_id id;
  if (ioctl(dev.native_handle(), EVIOCGID, &id) == -1)
    throw std::system_error{errno, std::system_category(), "ioctl(EVIOCGID)"};
  return id;
}

std::string load_dev_name(asio::posix::stream_descriptor& dev) {
  std::string name;
  name.resize(256);
  if (ioctl(dev.native_handle(), EVIOCGNAME(name.size() - 1), name.data()) ==
      -1)
    throw std::system_error{errno, std::system_category(), "ioctl(EVIOCGNAME)"};
  name.resize(std::strlen(name.data()));
  return name;
}

} // namespace evio
