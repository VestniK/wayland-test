#pragma once

#include <fcntl.h>
#include <sys/mman.h>


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

class mem_mapping {
public:
  mem_mapping() = default;
  ~mem_mapping() {unmap();}

  explicit mem_mapping(std::byte* data, size_t length): data_(data), length_(length) {}

  mem_mapping(const mem_mapping&) = delete;
  mem_mapping& operator= (const mem_mapping&) = delete;

  mem_mapping(mem_mapping&& rhs): data_(std::exchange(rhs.data_, nullptr)), length_(std::exchange(rhs.length_, 0)) {}
  mem_mapping& operator= (mem_mapping&& rhs) {
    unmap();
    data_ = std::exchange(rhs.data_, nullptr);
    length_ = std::exchange(rhs.length_, 0);
    return *this;
  }

  explicit operator bool () const noexcept {return data_ != nullptr;}

  const std::byte* data() const noexcept {return data_;}
  std::byte* data() noexcept {return data_;}

  size_t size() const noexcept {return length_;}

private:
  void unmap() noexcept {
    if (data_)
    {
      ::munmap(data_, length_);
    }
  }

private:
  std::byte* data_ = nullptr;
  size_t length_ = 0;
};

enum class prot: int {exec = PROT_EXEC, read = PROT_READ, write = PROT_WRITE, none = PROT_NONE};
constexpr bitmask<prot> operator | (prot lhs, prot rhs) noexcept {return bitmask<prot>{lhs} | rhs;}

enum class map: int {shared = MAP_SHARED, priv = MAP_PRIVATE, anon = MAP_ANONYMOUS};
constexpr bitmask<map> operator | (map lhs, map rhs) noexcept {return bitmask<map>{lhs} | rhs;}

inline
mem_mapping mmap(
  size_t length, bitmask<prot> prt, bitmask<map> flags,
  const file_descriptor& fd, std::streamoff off,
  std::error_code& ec
) noexcept {
  void* res = ::mmap(nullptr, length, prt.value(), flags.value(), fd.native_handle(), static_cast<off_t>(off));
  if (res == MAP_FAILED) {
    ec = {errno, std::system_category()};
    return {};
  }
  ec.clear();
  return mem_mapping{reinterpret_cast<std::byte*>(res), length};
}

inline
mem_mapping mmap(size_t length, bitmask<prot> prt, bitmask<map> flags, const file_descriptor& fd, std::streamoff off = 0) {
  std::error_code ec;
  mem_mapping res = mmap(length, prt, flags, fd, off, ec);
  if (ec)
    throw std::system_error{ec, "mmap"};
  return res;
}

} // namespace io
