#pragma once

#include <fcntl.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <unistd.h>

#include <span>
#include <string_view>
#include <system_error>
#include <utility>

#include <filesystem>

#include <util/bitmask.hpp>

namespace fs = std::filesystem;

namespace io {

class file_descriptor {
  static constexpr int invalid = -1;

public:
  using native_handle_t = int;

  constexpr file_descriptor() noexcept = default;
  constexpr explicit file_descriptor(native_handle_t fd) noexcept : fd_(fd) {}

  file_descriptor(const file_descriptor&) = delete;
  file_descriptor& operator=(const file_descriptor&) = delete;

  constexpr file_descriptor(file_descriptor&& rhs) noexcept : fd_(rhs.fd_) {
    rhs.fd_ = invalid;
  }
  constexpr file_descriptor& operator=(file_descriptor&& rhs) noexcept {
    if (fd_ >= 0)
      ::close(fd_);
    fd_ = rhs.fd_;
    rhs.fd_ = invalid;
    return *this;
  }

  ~file_descriptor() noexcept {
    if (fd_ >= 0)
      ::close(fd_);
  }

  void close() noexcept {
    if (fd_ >= 0)
      ::close(std::exchange(fd_, invalid));
  }
  constexpr explicit operator bool() const noexcept { return fd_ != invalid; }
  constexpr native_handle_t native_handle() const noexcept { return fd_; }
  constexpr native_handle_t release() noexcept {
    return std::exchange(fd_, invalid);
  }

  void lock() { flock(LOCK_EX); }
  bool try_lock() { return flock_nb(LOCK_EX); }
  void lock_shared() { flock(LOCK_SH); }
  bool try_lock_shared() { return flock_nb(LOCK_SH); }
  void unlock() { flock(LOCK_UN); }
  void unlock_shared() { flock(LOCK_UN); }

private:
  void flock(int op) {
    int res;
    do
      res = ::flock(fd_, op);
    while (res != 0 && errno == EINTR);
    if (res != 0)
      throw std::system_error{errno, std::system_category(), "flock"};
  }

  bool flock_nb(int op) {
    int res;
    do
      res = ::flock(fd_, op | LOCK_NB);
    while (res != 0 && errno == EINTR);
    return res == 0               ? true
           : errno == EWOULDBLOCK ? false
                                  : throw std::system_error{
                                        errno, std::system_category(), "flock"};
  }

private:
  native_handle_t fd_ = invalid;
};

enum class mode : int {
  create = O_CREAT,
  write_only = O_WRONLY,
  read_only = O_RDONLY,
  read_write = O_RDWR,
  truncate = O_TRUNC,
  tmpfile = O_TMPFILE,
  ndelay = O_NDELAY,
  nonblock = O_NONBLOCK,
};
constexpr bitmask<mode> operator|(mode lhs, mode rhs) noexcept {
  return bitmask<mode>{lhs} | rhs;
}

constexpr fs::perms default_perms =
    fs::perms::owner_read | fs::perms::owner_write | fs::perms::group_read |
    fs::perms::others_read;

inline file_descriptor open(const fs::path& path, bitmask<mode> flags,
    fs::perms perms = default_perms) {
  int fd = -1;
  do
    fd = ::open(path.c_str(), flags.value(), perms);
  while (fd < 0 && errno == EINTR);
  if (fd < 0)
    throw std::system_error{
        errno, std::system_category(), "open " + path.string()};
  return file_descriptor{fd};
}
inline file_descriptor open_anonymous(
    const fs::path& dir, bitmask<mode> flags, fs::perms perms = default_perms) {
  return open(dir, flags | mode::tmpfile, perms);
}

inline size_t read(const file_descriptor& fd, std::span<std::byte> buf,
    std::error_code& ec) noexcept {
  ssize_t res;
  do
    res = ::read(fd.native_handle(), buf.data(), buf.size());
  while (res < 0 && errno == EINTR);
  if (res < 0) {
    ec = {errno, std::system_category()};
    return 0;
  }
  return static_cast<size_t>(res);
}

inline size_t read(const file_descriptor& fd, std::span<std::byte> buf) {
  std::error_code ec;
  size_t res = read(fd, buf, ec);
  if (ec)
    throw std::system_error(ec, "write");
  return res;
}

inline size_t write(const file_descriptor& fd, std::span<const std::byte> data,
    std::error_code& ec) noexcept {
  ssize_t res;
  do
    res = ::write(fd.native_handle(), data.data(), data.size());
  while (res < 0 && errno == EINTR);
  if (res < 0) {
    ec = {errno, std::system_category()};
    return 0;
  }
  return static_cast<size_t>(res);
}
inline size_t write(
    const file_descriptor& fd, std::span<const std::byte> data) {
  std::error_code ec;
  size_t res = write(fd, data, ec);
  if (ec)
    throw std::system_error(ec, "write");
  return res;
}

inline size_t write(const file_descriptor& fd, std::string_view str,
    std::error_code ec) noexcept {
  return write(fd, std::as_bytes(std::span<const char>{str}), ec);
}

inline size_t write(const file_descriptor& fd, std::string_view str) {
  return write(fd, std::as_bytes(std::span<const char>{str}));
}

inline void sync(const file_descriptor& fd, std::error_code& ec) noexcept {
  int res;
  do
    res = ::fsync(fd.native_handle());
  while (res < 0 && errno == EINTR);
  if (res < 0)
    ec = {errno, std::system_category()};
}
inline void sync(const file_descriptor& fd) {
  std::error_code ec;
  sync(fd, ec);
  if (ec)
    throw std::system_error{ec, "fsync"};
}

inline void truncate(const file_descriptor& fd, std::streamoff off,
    std::error_code& ec) noexcept {
  int res;
  do
    res = ::ftruncate(fd.native_handle(), static_cast<off_t>(off));
  while (res < 0 && errno == EINTR);
  if (res < 0)
    ec = {errno, std::system_category()};
}
inline void truncate(const file_descriptor& fd, std::streamoff off) {
  std::error_code ec;
  truncate(fd, off, ec);
  if (ec)
    throw std::system_error{ec, "ftruncate"};
}

class transactional_file {
public:
  transactional_file() noexcept = default;
  transactional_file(const fs::path& path, bitmask<io::mode> mode,
      fs::perms perms = io::default_perms)
      : fd_{io::open_anonymous(path.parent_path(), mode, perms)},
        dest_path_{path} {}

  operator const file_descriptor&() const noexcept { return fd_; }

  void commit() {
    sync(fd_);
    std::array<char, 64> buf;
    ::snprintf(buf.data(), buf.size(), "/proc/self/fd/%d",
        static_cast<int>(fd_.native_handle()));
    if (::linkat(AT_FDCWD, buf.data(), AT_FDCWD, dest_path_.c_str(),
            AT_SYMLINK_FOLLOW) < 0)
      throw std::system_error{errno, std::system_category(), "linkat"};
  }

private:
  file_descriptor fd_;
  fs::path dest_path_;
};

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
auto mmap(const file_descriptor& fd, std::streamoff start, size_t len) {
  std::error_code ec;
  auto res = mmap(fd, start, len, ec);
  if (ec)
    throw std::system_error{ec, "mmap"};
  return res;
}

const std::byte* data(
    const std::unique_ptr<const std::byte[], auto_unmaper>& mapping) noexcept {
  return mapping.get();
}

size_t size(
    const std::unique_ptr<const std::byte[], auto_unmaper>& mapping) noexcept {
  return mapping.get_deleter().size;
}

const std::byte* begin(
    const std::unique_ptr<const std::byte[], auto_unmaper>& mapping) noexcept {
  return data(mapping);
}

const std::byte* end(
    const std::unique_ptr<const std::byte[], auto_unmaper>& mapping) noexcept {
  return data(mapping) + size(mapping);
}

} // namespace io
