#include "arena.impl.hpp"

#include <memory>

#include <catch2/catch_test_macros.hpp>

class typed_memory : public std::unique_ptr<std::byte[]> {
public:
  typed_memory() noexcept = default;
  typed_memory(uint32_t memtype, size_t sz)
      : std::unique_ptr<std::byte[]>{new std::byte[sz]}, size_{sz},
        memtype_{memtype} {}

  std::byte* data() noexcept { return get(); }
  const std::byte* data() const noexcept { return get(); }
  size_t size() const noexcept { return size_; }

  std::byte* begin() noexcept { return get(); }
  const std::byte* begin() const noexcept { return get(); }
  const std::byte* end() const noexcept { return get() + size_; }

  uint32_t type() const noexcept { return memtype_; }

private:
  size_t size_ = 0;
  uint32_t memtype_ = 0;
};

SCENARIO("Locking available amount of bytes") {
  GIVEN("some nonempty arenas_pool") {
    vlk::arena_pools<typed_memory> arenas{{.vbo_capacity = 100,
                                              .ibo_capacity = 100,
                                              .textures_capacity = 500,
                                              .staging_size = 0},
        [](auto purpose [[maybe_unused]]) {
          return vk::MemoryRequirements{
              .size = 1 << 30, .alignment = 8, .memoryTypeBits = 100};
        },
        [](uint32_t memtype, size_t sz) { return typed_memory{memtype, sz}; }};

    WHEN("available amount of bytes are requested to be locked for any buffer "
         "purpose") {
      auto [mem, region] = arenas.lock_memory_for(vlk::buffer_purpose::ibo, 30);

      THEN("requested amount of bytes locked") { REQUIRE(region.len == 30); }

      THEN("locked region is properly aligned") {
        REQUIRE(region.offset % 8 == 0);
      }

      THEN("locks memory of ptoper type") { REQUIRE(mem.type() == 100); }
    }
  }
}
