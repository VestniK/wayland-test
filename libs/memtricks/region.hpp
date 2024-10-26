#pragma once

#include <cstddef>
#include <span>

struct memory_region {
  size_t offset = 0;
  size_t len = 0;
};

inline memory_region subspan_region(
    std::span<const std::byte> span, std::span<const std::byte> subspan) {
  return {.offset = static_cast<size_t>(subspan.data() - span.data()),
      .len = subspan.size()};
}
