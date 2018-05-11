#pragma once

#include <fcntl.h>

#include <system_error>
#include <string_view>

#include <experimental/filesystem>

#include "bitmask.h"

namespace fs = std::experimental::filesystem;

namespace io {

class file_descriptor {
  static constexpr int invalid = -1;
public:
  file_descriptor() noexcept = default;
  explicit file_descriptor(int fd) noexcept: fd_(fd) {}

  file_descriptor(const file_descriptor&) = delete;
  file_descriptor& operator= (const file_descriptor&) = delete;

  file_descriptor(file_descriptor&& rhs) noexcept;
  file_descriptor& operator= (file_descriptor&& rhs) noexcept;

  ~file_descriptor() noexcept;

  void close() noexcept;
  explicit operator bool () const noexcept {return fd_ != invalid;}
  int native_handle() const noexcept {return fd_;}

private:
  int fd_ = invalid;
};

enum class mode: int {
  create = O_CREAT,
  write_only = O_WRONLY,
  read_only = O_RDONLY,
  read_write = O_RDWR,
  truncate = O_TRUNC,
  tmpfile = O_TMPFILE
};
constexpr bitmask<mode> operator| (mode lhs, mode rhs) noexcept {
  return bitmask<mode>{lhs} | rhs;
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
constexpr bitmask<perm> operator| (perm lhs, perm rhs) noexcept {
  return bitmask<perm>{lhs} | rhs;
}
constexpr bitmask<perm> default_perms = perm::owner_read | perm::owner_write | perm::grp_read | perm::other_read;

file_descriptor open(const fs::path& path, bitmask<mode> flags, bitmask<perm> perms = default_perms);

void sync(const file_descriptor& fd, std::error_code& ec) noexcept;
void sync(const file_descriptor& fd);

void truncate(const file_descriptor& fd, std::streamoff off, std::error_code& ec) noexcept;
void truncate(const file_descriptor& fd, std::streamoff off);

} // namespace io
