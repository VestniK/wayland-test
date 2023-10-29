#pragma once

#include <linux/input.h>

#include <string>

#include <asio/posix/stream_descriptor.hpp>

#include <gamepad/evio_codes.hpp>
#include <gamepad/types/axis.hpp>

namespace evio {

std::expected<input_absinfo, evio::error> load_absinfo(
    asio::posix::stream_descriptor& dev, gamepad::axis axis,
    gamepad::dimention coord);

input_id load_dev_id(asio::posix::stream_descriptor& dev);

std::string load_dev_name(asio::posix::stream_descriptor& dev);

} // namespace evio
