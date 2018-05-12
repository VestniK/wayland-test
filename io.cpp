#include <unistd.h>

#include <utility>

#include "io.h"

namespace io {

file_descriptor::file_descriptor(file_descriptor&& rhs) noexcept:
  fd_(std::exchange(rhs.fd_, invalid))
{}

file_descriptor& file_descriptor::operator= (file_descriptor&& rhs) noexcept {
  close();
  fd_ = std::exchange(rhs.fd_, invalid);
  return *this;
}

file_descriptor::~file_descriptor() noexcept {
  if (fd_ >= 0)
    ::close(fd_);
}

void file_descriptor::close() noexcept {
  if (const int fd = std::exchange(fd_, invalid); fd >= 0)
    ::close(fd);
}

file_descriptor open(const fs::path& path, wl::bitmask<mode> flags, wl::bitmask<perm> perms) {
  int fd = ::open(path.c_str(), flags.value(), perms.value());
  if (fd < 0)
    throw std::system_error{errno, std::system_category(), "open " + path.string()};
  return file_descriptor{fd};
}

void sync(const file_descriptor& fd, std::error_code& ec) noexcept {
  int res = ::fsync(fd.native_handle());
  if (res < 0)
    ec = {errno, std::system_category()};
}

void sync(const file_descriptor& fd) {
  std::error_code ec;
  do sync(fd, ec); while (ec == std::errc::interrupted);
  if (ec)
    throw std::system_error{ec, "fsync"};
}

void truncate(const file_descriptor& fd, std::streamoff off, std::error_code& ec) noexcept {
  int res = ::ftruncate(fd.native_handle(), static_cast<off_t>(off));
  if (res < 0)
    ec = {errno, std::system_category()};
}

void truncate(const file_descriptor& fd, std::streamoff off) {
  std::error_code ec;
  do truncate(fd, off, ec); while (ec == std::errc::interrupted);
  if (ec)
    throw std::system_error{ec, "fsync"};
}

} // namespace io
