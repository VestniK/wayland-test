#pragma once

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <utility>
#include <system_error>

#include <experimental/filesystem>

#include <util/bitmask.hpp>

namespace fs = std::experimental::filesystem;

namespace io {

class file_descriptor {
  static constexpr int invalid = -1;
public:
  file_descriptor() noexcept = default;
  explicit file_descriptor(int fd) noexcept: fd_(fd) {}

  file_descriptor(const file_descriptor&) = delete;
  file_descriptor& operator= (const file_descriptor&) = delete;

  file_descriptor(file_descriptor&& rhs) noexcept: fd_{std::exchange(rhs.fd_, invalid)} {}
  file_descriptor& operator= (file_descriptor&& rhs) noexcept {
    close();
    fd_ = std::exchange(rhs.fd_, invalid);
    return *this;
  }

  ~file_descriptor() noexcept {close();}

  void close() noexcept {if (fd_ > 0) ::close(fd_);}
  explicit operator bool () const noexcept {return fd_ != invalid;}
  int native_handle() const noexcept {return fd_;}

private:
  int fd_ = invalid;
};

enum class mode: int {
  create = O_CREAT,
  close_exec = O_CLOEXEC,
  write_only = O_WRONLY,
  read_only = O_RDONLY,
  read_write = O_RDWR,
  truncate = O_TRUNC,
  tmpfile = O_TMPFILE
};
constexpr ut::bitmask<mode> operator| (mode lhs, mode rhs) noexcept {
  return ut::bitmask<mode>{lhs} | rhs;
}

enum class perm: mode_t {
  owner_read = S_IRUSR,
  owner_write = S_IWUSR,
  owner_exec = S_IXUSR,

  grp_read = S_IRGRP,
  grp_write = S_IWGRP,
  grp_exec = S_IXGRP,

  other_read = S_IROTH,
  other_write = S_IWOTH,
  other_exec = S_IXOTH,
};
constexpr ut::bitmask<perm> operator| (perm lhs, perm rhs) noexcept {
  return ut::bitmask<perm>{lhs} | rhs;
}
constexpr ut::bitmask<perm> default_perms = perm::owner_read | perm::owner_write | perm::grp_read | perm::other_read;

inline
file_descriptor open(const fs::path& path, ut::bitmask<mode> flags, ut::bitmask<perm> perms = default_perms) {
  int fd = ::open(path.c_str(), flags.value(), perms.value());
  if (fd < 0)
    throw std::system_error{errno, std::system_category(), "open " + path.string()};
  return file_descriptor{fd};
}

inline
void sync(const file_descriptor& fd, std::error_code& ec) noexcept {
  int res = ::fsync(fd.native_handle());
  if (res < 0)
    ec = {errno, std::system_category()};
}
inline
void sync(const file_descriptor& fd) {
  std::error_code ec;
  do sync(fd, ec); while (ec == std::errc::interrupted);
  if (ec)
    throw std::system_error{ec, "fsync"};
}

inline
void truncate(const file_descriptor& fd, std::streamoff off, std::error_code& ec) noexcept {
  int res = ::ftruncate(fd.native_handle(), static_cast<off_t>(off));
  if (res < 0)
    ec = {errno, std::system_category()};
}
inline
void truncate(const file_descriptor& fd, std::streamoff off) {
  std::error_code ec;
  do truncate(fd, off, ec); while (ec == std::errc::interrupted);
  if (ec)
    throw std::system_error{ec, "fsync"};
}

} // namespace io
