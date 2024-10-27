#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>

#include <utility>
#include <vulkan/vulkan.hpp>

#include <libs/memtricks/region.hpp>

namespace vlk {

enum class buffer_purpose { vbo, ibo };
enum class image_purpose {
  texture = std::to_underlying(buffer_purpose::ibo) + 1
};

template <typename E>
concept memory_purpose =
    std::same_as<E, buffer_purpose> || std::same_as<E, image_purpose>;

template <memory_purpose Purpose>
struct purpose_traits;

template <>
struct purpose_traits<buffer_purpose> {
  static constexpr size_t purposes_count = 2;
  static constexpr buffer_purpose values[] = {
      buffer_purpose::vbo, buffer_purpose::ibo};
};

template <>
struct purpose_traits<image_purpose> {
  static constexpr size_t purposes_count = 1;
  static constexpr image_purpose values[] = {image_purpose::texture};
};

template <memory_purpose Purpose>
inline constexpr size_t purposes_count =
    purpose_traits<Purpose>::purposes_count;

template <memory_purpose Purpose>
inline constexpr auto& all_purposes = purpose_traits<Purpose>::values;

constexpr size_t purpose_idx(memory_purpose auto purpose) noexcept {
  return static_cast<size_t>(purpose);
}

namespace detail {

inline constexpr size_t purpose_data_arity =
    purposes_count<buffer_purpose> + purposes_count<image_purpose>;

template <typename V>
struct purpose_data : std::array<V, purpose_data_arity> {
  V& operator[](memory_purpose auto idx) noexcept {
    return (*this)[purpose_idx(idx)];
  }
  const V& operator[](memory_purpose auto idx) const noexcept {
    return (*this)[purpose_idx(idx)];
  }

  using std::array<V, purpose_data_arity>::operator[];
};

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

  purpose_data<const Memory*> get_memory_handles() const noexcept;

private:
  struct arena_info {
    size_t used;
    size_t capacity;
    size_t alignment;
    uint32_t pool_idx;
  };

  purpose_data<arena_info> arena_infos_;
  std::array<Memory, purpose_data_arity> pools_;
};

} // namespace detail

} // namespace vlk
