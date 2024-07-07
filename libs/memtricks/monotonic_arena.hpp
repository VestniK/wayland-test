#pragma once

#include <memory>
#include <ranges>
#include <utility>

namespace detail {
template <typename T>
struct non_deleting_deleter {
  void operator()(T* ptr) noexcept { ptr->~T(); }
};
} // namespace detail

template <typename T>
concept arena_buffer =
    std::ranges::contiguous_range<T> && std::ranges::sized_range<T> &&
    std::same_as<std::ranges::range_value_t<T>, std::byte>;

template <arena_buffer Src>
class monotonic_arena {
public:
  template <typename T>
  using deleter = detail::non_deleting_deleter<T>;

  template <typename T>
  using unique_ptr = std::unique_ptr<T, deleter<T>>;

public:
  monotonic_arena() noexcept(
      std::is_nothrow_default_constructible_v<Src>) = default;

  monotonic_arena(Src&& src) noexcept : mem_source_{std::move(src)} {}

  monotonic_arena(const monotonic_arena&) = delete;
  monotonic_arena& operator=(const monotonic_arena&) = delete;
  monotonic_arena(monotonic_arena&&) = default;
  monotonic_arena& operator=(monotonic_arena&&) = default;
  ~monotonic_arena() noexcept = default;

  void clear() noexcept { offset_ = 0; }

  void* allocate(
      std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) {
    auto alloc = find_mem_for(bytes, alignment);
    commit_allocation(alloc);
    return alloc.data();
  }

  size_t capacity() noexcept { return mem_source_.size() - offset_; }

  template <typename T, typename... A>
    requires std::constructible_from<T, A&&...>
  unique_ptr<T> aligned_allocate_unique(std::align_val_t align, A&&... a) {
    auto alloc = find_mem_for(sizeof(T), std::to_underlying(align));
    unique_ptr<T> res{new (alloc.data()) T{std::forward<A>(a)...}};
    commit_allocation(alloc);
    return res;
  }

  template <typename T, typename... A>
    requires std::constructible_from<T, A&&...>
  unique_ptr<T> allocate_unique(A&&... a) {
    return aligned_allocate_unique<T>(
        static_cast<std::align_val_t>(
            std::max(alignof(T), alignof(std::max_align_t))),
        std::forward<A>(a)...);
  }

private:
  std::span<std::byte> find_mem_for(std::size_t sz, std::size_t align) {
    void* res = mem_source_.data() + offset_;
    size_t remains = capacity();
    if (!std::align(align, sz, res, remains))
      throw std::bad_alloc{};
    return {static_cast<std::byte*>(res), sz};
  }

  void commit_allocation(std::span<std::byte> alloc) {
    offset_ = alloc.data() - mem_source_.data() + alloc.size();
  }

private:
  Src mem_source_;
  size_t offset_ = 0;
};
