#include "framebuf.hpp"

#include <libs/xdg/xdg.hpp>

namespace wl {

framebuf::framebuf(wl_shm& shm, size sz) {
  const size_t pixel_size = 4 * sz.width * sz.height;

  auto mapping_fd = thinsys::io::open_anonymous(xdg::runtime_dir(), thinsys::io::mode::read_write);
  thinsys::io::truncate(mapping_fd, 2 * pixel_size);

  shmem_ = thinsys::io::mmap_mut(mapping_fd, 0, 2 * pixel_size, thinsys::io::map_sharing::sahre);
  spool_ = wl::unique_ptr<wl_shm_pool>{wl_shm_create_pool(&shm, mapping_fd.native_handle(), 2 * pixel_size)};
  bufs_ = {
      wl::unique_ptr<wl_buffer>{wl_shm_pool_create_buffer(
          spool_.get(), 0, sz.width, sz.height, 4 * sz.width, WL_SHM_FORMAT_ARGB8888
      )},
      wl::unique_ptr<wl_buffer>{wl_shm_pool_create_buffer(
          spool_.get(), pixel_size, sz.width, sz.height, 4 * sz.width, WL_SHM_FORMAT_ARGB8888
      )}
  };
}

void framebuf::swap(wl_surface& surf) {
  wl_surface_attach(&surf, bufs_[cur_].get(), 0, 0);
  wl_surface_commit(&surf);
  cur_ = (cur_ + 1) % 2;
}

std::span<std::byte> framebuf::front() const noexcept {
  auto res = std::span{shmem_};
  return res.subspan(cur_ * res.size() / 2, res.size() / 2);
}

} // namespace wl
