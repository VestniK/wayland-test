#pragma once

#include "wlutil.hpp"

#include <thinsys/io/mmap.hpp>

#include <span>

namespace wl {

class framebuf {
public:
  framebuf() noexcept = default;
  framebuf(wl_shm& shm, size sz);

  void swap(wl_surface& surf);
  std::span<std::byte> front() const noexcept;

private:
  std::unique_ptr<std::byte[], thinsys::io::auto_unmaper> shmem_;
  wl::unique_ptr<wl_shm_pool> spool_;
  std::array<wl::unique_ptr<wl_buffer>, 2> bufs_;
  size_t cur_ = 0;
};

} // namespace wl
