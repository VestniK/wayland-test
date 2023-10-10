#pragma once

#include <concepts>
#include <span>
#include <system_error>
#include <utility>

namespace io {

template <typename T>
struct input_traits {
  static size_t read(
      T& in, std::span<std::byte> dest, std::error_code& ec) noexcept = delete;
};

template <typename T>
concept input =
    requires(T& in, std::span<std::byte> dest, std::error_code& ec) {
      { input_traits<T>::read(in, dest, ec) } -> std::same_as<size_t>;
    } && std::movable<T>;

// Repeates successive raw read untill buffer is filled or EOF reached
template <input In>
size_t read(In& in, std::span<std::byte> dest, std::error_code& ec) noexcept {
  size_t res = 0;
  while (!dest.empty()) {
    const auto sz = input_traits<In>::read(in, dest, ec);
    if (ec || sz == 0)
      break;
    res += sz;
    dest = dest.subspan(sz);
  }
  return res;
}

template <input In>
size_t read(In& in, std::span<std::byte> dest) {
  std::error_code ec;
  size_t res = ::io::read<In>(in, dest, ec);
  if (ec)
    throw std::system_error(ec, "read");
  return res;
}

} // namespace io
