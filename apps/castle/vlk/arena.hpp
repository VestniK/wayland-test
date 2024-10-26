#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>

#include <vulkan/vulkan.hpp>

#include <libs/memtricks/region.hpp>

namespace vlk {

enum class buffer_purpose { vbo, ibo };
enum class image_purpose { texture };

template <typename E>
concept memory_purpose =
    std::same_as<E, buffer_purpose> || std::same_as<E, image_purpose>;

template <memory_purpose Purpose>
struct purpose_traits;

template <>
struct purpose_traits<buffer_purpose> {
  static constexpr size_t purposes_count = 2;
};

template <>
struct purpose_traits<image_purpose> {
  static constexpr size_t purposes_count = 1;
};

template <memory_purpose Purpose>
inline constexpr size_t purposes_count =
    purpose_traits<Purpose>::purposes_count;

namespace detail {

template <typename F>
concept query_memreq_function =
    requires(F&& f, buffer_purpose bp, image_purpose ip) {
      { f(bp) } -> std::convertible_to<vk::MemoryRequirements>;
      { f(ip) } -> std::convertible_to<vk::MemoryRequirements>;
    };

template <typename F, typename Memory>
concept device_allocation_function =
    requires(F&& f, uint32_t mem_type_bits, size_t sz) {
      { f(mem_type_bits, sz) } -> std::same_as<Memory>;
    };

struct sizes {
  size_t vbo_capacity;
  size_t ibo_capacity;
  size_t textures_capacity;
  size_t staging_size;
};

template <std::default_initializable Memory>
class arena_pools {
public:
  using enum buffer_purpose;
  using enum image_purpose;

  template <query_memreq_function QueryReq,
      device_allocation_function<Memory> AllocMem>
  arena_pools(sizes pool_sizes, QueryReq&& query_req_fn, AllocMem&& alloc_fn);

  template <memory_purpose Purpose>
  std::tuple<Memory&, memory_region> lock_memory_for(Purpose p, size_t sz);

private:
  static size_t as_idx(buffer_purpose p) noexcept {
    return static_cast<size_t>(p);
  }

  static size_t as_idx(image_purpose p) noexcept {
    return static_cast<size_t>(p) + purposes_count<buffer_purpose>;
  }

  struct arena_info {
    size_t used;
    size_t capacity;
    size_t alignment;
    uint32_t pool_idx;
  };

  constexpr static size_t arenas_count =
      purposes_count<buffer_purpose> + purposes_count<image_purpose>;

  std::array<arena_info, arenas_count> arena_infos_;
  std::array<Memory, arenas_count> pools_;
};

} // namespace detail

} // namespace vlk
