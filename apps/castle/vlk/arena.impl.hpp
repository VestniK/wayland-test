#pragma once

#include <algorithm>
#include <optional>
#include <ranges>
#include <utility>

#include "arena.hpp"

namespace vlk {

template <std::default_initializable Memory>
template <query_memreq_function QueryReq,
    device_allocation_function<Memory> AllocMem>
arena_pools<Memory>::arena_pools(
    sizes pool_sizes, QueryReq&& query_req_fn, AllocMem&& alloc_fn) {
  std::array<std::tuple<vk::MemoryRequirements, size_t, size_t>, arenas_count>
      type2purpose{
          std::tuple{query_req_fn(vbo), as_idx(vbo), pool_sizes.vbo_capacity},
          std::tuple{query_req_fn(ibo), as_idx(ibo), pool_sizes.ibo_capacity},
          std::tuple{query_req_fn(texture), as_idx(texture),
              pool_sizes.textures_capacity},
      };

  std::ranges::sort(type2purpose, [](auto l, auto r) {
    return std::get<0>(l).memoryTypeBits < std::get<0>(r).memoryTypeBits;
  });

  std::optional<uint32_t> last_type_bits;
  uint32_t next_mem_slot_idx = 0;
  size_t total_capacity = 0;
  for (auto [mem_req, purp, capacity] : type2purpose) {
    if (const auto prev = std::exchange(last_type_bits, mem_req.memoryTypeBits);
        prev && prev != mem_req.memoryTypeBits) {
      pools_[next_mem_slot_idx++] =
          alloc_fn(mem_req.memoryTypeBits, std::exchange(total_capacity, 0));
    }
    total_capacity += capacity;
    arena_infos_[purp] = {
        .used = 0,
        .capacity = capacity,
        .alignment = mem_req.alignment,
        .pool_idx = next_mem_slot_idx,
    };
  }
  pools_[next_mem_slot_idx] = alloc_fn(*last_type_bits, total_capacity);
}

template <std::default_initializable Memory>
std::tuple<Memory&, memory_region> arena_pools<Memory>::lock_memory_for(
    buffer_purpose p, size_t sz) {
  arena_info& arena = arena_infos_[as_idx(p)];
  const size_t offset = std::ranges::fold_left(
      arena_infos_ | std::views::filter([&arena](auto&& item) {
        return item.pool_idx == arena.pool_idx;
      }) | std::views::transform(&arena_info::used),
      size_t{0}, std::plus{});
  const size_t padding =
      (arena.alignment - offset % arena.alignment) % arena.alignment;
  if (arena.capacity - arena.used < sz + padding)
    throw std::bad_alloc{};
  arena.used = arena.used + sz + padding;
  return {pools_[arena.pool_idx],
      memory_region{.offset = offset + padding, .len = sz}};
}

} // namespace vlk
