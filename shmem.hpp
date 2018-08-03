#pragma once

#include "file.hpp"
#include "xdg.hpp"

namespace io {

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
constexpr ut::bitmask<prot> operator | (prot lhs, prot rhs) noexcept {return ut::bitmask<prot>{lhs} | rhs;}

enum class map: int {shared = MAP_SHARED, priv = MAP_PRIVATE, anon = MAP_ANONYMOUS};
constexpr ut::bitmask<map> operator | (map lhs, map rhs) noexcept {return ut::bitmask<map>{lhs} | rhs;}

inline
mem_mapping mmap(
  size_t length, ut::bitmask<prot> prt, ut::bitmask<map> flags,
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
mem_mapping mmap(
  size_t length, ut::bitmask<prot> prt, ut::bitmask<map> flags, const file_descriptor& fd, std::streamoff off = 0
) {
  std::error_code ec;
  mem_mapping res = mmap(length, prt, flags, fd, off, ec);
  if (ec)
    throw std::system_error{ec, "mmap"};
  return res;
}

class shared_memory {
public:
  shared_memory(io::file_descriptor fd, size_t size): fd_(std::move(fd)) {
    io::truncate(fd_, size);
    mem_ = io::mmap(size, io::prot::read | io::prot::write, io::map::shared, fd_);
  }

  explicit shared_memory(size_t size):
    shared_memory(io::open(xdg::runtime_dir(), io::mode::read_write | io::mode::tmpfile | io::mode::close_exec), size)
  {}

  std::byte* data() noexcept {return mem_.data();}
  const std::byte* data() const noexcept {return mem_.data();}
  size_t size() const noexcept {return mem_.size();}

  const io::file_descriptor& fd() const noexcept {return fd_;}

private:
  io::file_descriptor fd_;
  io::mem_mapping mem_;
};

}
