#pragma once

#include <algorithm>
#include <optional>
#include <ranges>
#include <utility>

#include "arena.hpp"

namespace vlk {

namespace detail {

template <std::default_initializable Memory>
template <query_memreq_function QueryReq, device_allocation_function<Memory> AllocMem>
arena_pools<Memory>::arena_pools(sizes pool_sizes, QueryReq&& query_req_fn, AllocMem&& alloc_fn) {
  std::array<std::tuple<vk::MemoryRequirements, size_t, size_t>, purpose_data_arity> type2purpose{
      std::tuple{query_req_fn(vbo), purpose_idx(vbo), pool_sizes.vbo_capacity},
      std::tuple{query_req_fn(ibo), purpose_idx(ibo), pool_sizes.ibo_capacity},
      std::tuple{query_req_fn(texture), purpose_idx(texture), pool_sizes.textures_capacity},
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
      pools_[next_mem_slot_idx++] = alloc_fn(*prev, std::exchange(total_capacity, 0));
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
template <memory_purpose Purpose>
std::tuple<Memory&, memory_region> arena_pools<Memory>::lock_memory_for(Purpose p, size_t sz) {
  return lock_memory_for(p, sz, 0);
}

template <std::default_initializable Memory>
template <memory_purpose Purpose>
std::tuple<Memory&, memory_region>
arena_pools<Memory>::lock_memory_for(Purpose p, size_t sz, size_t alignment) {
  arena_info& arena = arena_infos_[p];
  const size_t offset = std::ranges::fold_left(
      arena_infos_ | std::views::filter([&arena](auto&& item) { return item.pool_idx == arena.pool_idx; }) |
          std::views::transform(&arena_info::used),
      size_t{0}, std::plus{}
  );
  const size_t align = std::max(alignment, arena.alignment);
  const size_t padding = align > 0 ? (align - offset % align) % align : 0;
  if (arena.capacity - arena.used < sz + padding)
    throw std::bad_alloc{};
  arena.used = arena.used + sz + padding;
  return {pools_[arena.pool_idx], memory_region{.offset = offset + padding, .len = sz}};
}

template <std::default_initializable Memory>
purpose_data<const Memory*> arena_pools<Memory>::get_memory_handles() const noexcept {
  purpose_data<const Memory*> res;
  for (const auto& [idx, info] : arena_infos_ | std::views::enumerate) {
    res[idx] = &pools_[info.pool_idx];
  }
  return res;
}

} // namespace detail

} // namespace vlk
