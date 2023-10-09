#pragma once

#include <memory>

#include <util/io/io.hpp>

namespace io {

struct auto_unmaper {
  size_t size{0};

  void operator()(const std::byte* buf) noexcept {
    if (buf)
      ::munmap(const_cast<std::byte*>(buf), size);
  }
};

inline std::unique_ptr<const std::byte[], auto_unmaper> mmap(
    const file_descriptor& fd, std::streamoff start, size_t len,
    std::error_code& ec) noexcept {
  std::byte* data = reinterpret_cast<std::byte*>(::mmap(nullptr, len, PROT_READ,
      MAP_PRIVATE, fd.native_handle(), static_cast<off_t>(start)));
  if (data == MAP_FAILED) {
    ec = {errno, std::system_category()};
    return {};
  }
  return std::unique_ptr<const std::byte[], auto_unmaper>{
      data, auto_unmaper{len}};
}
inline auto mmap(const file_descriptor& fd, std::streamoff start, size_t len) {
  std::error_code ec;
  auto res = mmap(fd, start, len, ec);
  if (ec)
    throw std::system_error{ec, "mmap"};
  return res;
}

inline const std::byte* data(
    const std::unique_ptr<const std::byte[], auto_unmaper>& mapping) noexcept {
  return mapping.get();
}

inline size_t size(
    const std::unique_ptr<const std::byte[], auto_unmaper>& mapping) noexcept {
  return mapping.get_deleter().size;
}

inline const std::byte* begin(
    const std::unique_ptr<const std::byte[], auto_unmaper>& mapping) noexcept {
  return data(mapping);
}

inline const std::byte* end(
    const std::unique_ptr<const std::byte[], auto_unmaper>& mapping) noexcept {
  return data(mapping) + size(mapping);
}

} // namespace io
