#pragma once

#include <concepts>
#include <span>
#include <string_view>
#include <system_error>
#include <utility>

namespace io {

template <typename T>
struct output_traits {
  static size_t write(T& out, std::span<const std::byte> dest,
      std::error_code& ec) noexcept = delete;
};

template <typename T>
concept output =
    requires(T& out, std::span<const std::byte> dest, std::error_code& ec) {
      { output_traits<T>::write(out, dest, ec) } -> std::same_as<size_t>;
    } && std::movable<T>;

template <output Out>
void write(
    Out& in, std::span<const std::byte> buf, std::error_code& ec) noexcept {
  while (!buf.empty()) {
    const auto sz = output_traits<Out>::write(in, buf, ec);
    if (ec)
      break;
    buf = buf.subspan(sz);
  }
}

template <output Out>
void write(Out& out, std::span<std::byte> dest) {
  std::error_code ec;
  ::io::write<Out>(out, dest, ec);
  if (ec)
    throw std::system_error(ec, "write");
}

template <output Out>
void write(Out& out, std::string_view str, std::error_code ec) noexcept {
  write(out, std::as_bytes(std::span{str}), ec);
}

template <output Out>
void write(Out& out, std::string_view str) noexcept {
  write(out, std::as_bytes(std::span{str}));
}

} // namespace io
