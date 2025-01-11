#include "arena.impl.hpp"

#include <memory>

#include <fmt/format.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

using Catch::Generators::from_range;

namespace {

class typed_memory {
public:
  typed_memory() noexcept = default;
  typed_memory(uint32_t memtype, size_t sz) noexcept : size_{sz}, memtype_{memtype} {}

  typed_memory(const typed_memory&) = delete;
  typed_memory& operator=(const typed_memory&) = delete;

  typed_memory(typed_memory&&) noexcept = default;
  typed_memory& operator=(typed_memory&&) noexcept = default;
  ~typed_memory() noexcept = default;

  size_t size() const noexcept { return size_; }

  uint32_t type() const noexcept { return memtype_; }

  static typed_memory allocate(uint32_t memtype, size_t sz) noexcept { return {memtype, sz}; }

private:
  size_t size_ = 0;
  uint32_t memtype_ = 0;
};

struct test_mem_params {
  size_t alignment;
  uint32_t type;
};

struct test_mem_type {
  uint32_t type;

  test_mem_params operator()(size_t alignment) const noexcept {
    return {.alignment = alignment, .type = type};
  }
};

constexpr test_mem_type operator"" _mem_type(unsigned long long type) noexcept {
  return {.type = static_cast<uint32_t>(type)};
}

struct test_arena_prams {

  std::string_view description;
  vlk::detail::purpose_data<test_mem_params> params;
};

test_arena_prams cases[] = {
    {.description = "all requirements same", .params{100_mem_type(8), 100_mem_type(8), 100_mem_type(8)}},
    {.description = "all requirements differs", .params{100_mem_type(8), 200_mem_type(16), 150_mem_type(32)}},
    {.description = "same requirements for buffers",
     .params{100_mem_type(8), 100_mem_type(16), 150_mem_type(32)}},
    {.description = "same requirements between some buffer and image purpose",
     .params{100_mem_type(16), 200_mem_type(16), 100_mem_type(32)}}
};

} // namespace

SCENARIO("Locking available amount of bytes") {
  auto case_params = GENERATE(from_range(cases));
  GIVEN(fmt::format("some nonempty arena_pools with {}", case_params.description)) {
    vlk::detail::arena_pools<typed_memory> arenas{
        {.vbo_capacity = 100, .ibo_capacity = 100, .textures_capacity = 500, .staging_size = 0},
        [&](vlk::memory_purpose auto purpose) {
          return vk::MemoryRequirements{
              1 << 30, case_params.params[purpose].alignment, case_params.params[purpose].type
          };
        },
        typed_memory::allocate
    };

    WHEN("available amount of bytes are locked for a buffer purpose") {
      const auto purpose = GENERATE(from_range(vlk::all_purposes<vlk::buffer_purpose>));
      const auto purpose_params = case_params.params[purpose];
      auto [mem, region] = arenas.lock_memory_for(purpose, 35);

      THEN("requested amount of bytes locked") { CHECK(region.len == 35); }

      THEN("locked region is properly aligned") { CHECK(region.offset % purpose_params.alignment == 0); }

      THEN("locks memory of ptoper type") { CHECK(mem.type() == purpose_params.type); }
    }

    WHEN("available amount of bytes are locked for an image purpose") {
      const auto purpose = GENERATE(from_range(vlk::all_purposes<vlk::image_purpose>));
      const auto purpose_params = case_params.params[purpose];
      auto [mem, region] = arenas.lock_memory_for(purpose, 25);

      THEN("requested amount of bytes locked") { CHECK(region.len == 25); }

      THEN("locked region is properly aligned") { CHECK(region.offset % purpose_params.alignment == 0); }

      THEN("locks memory of ptoper type") { CHECK(mem.type() == purpose_params.type); }
    }

    WHEN("available amount of bytes are locked for an image purpose with "
         "custom alignment requirements") {
      const auto purpose = GENERATE(from_range(vlk::all_purposes<vlk::image_purpose>));
      const auto purpose_params = case_params.params[purpose];
      auto [mem, region] = arenas.lock_memory_for(purpose, 35, 512);

      THEN("requested amount of bytes locked") { CHECK(region.len == 35); }

      THEN("locked region is properly aligned") { CHECK(region.offset % 512 == 0); }

      THEN("locks memory of ptoper type") { CHECK(mem.type() == purpose_params.type); }
    }
  }
}

SCENARIO("arena_pools creation") {
  using enum vlk::buffer_purpose;
  using enum vlk::image_purpose;

  auto req4type = [](uint32_t type) { return vk::MemoryRequirements{1 << 30, 8, type}; };
  vlk::detail::sizes arena_sizes{
      .vbo_capacity = 400, .ibo_capacity = 500, .textures_capacity = 600, .staging_size = 0
  };

  GIVEN("diferent mem requirements for diferent purpuses") {
    vlk::detail::purpose_data<vk::MemoryRequirements> requierments{
        req4type(100), req4type(200), req4type(300)
    };

    WHEN("arena_pools created") {
      vlk::detail::arena_pools<typed_memory> arenas{
          arena_sizes, [&](auto p) { return requierments[p]; }, typed_memory::allocate
      };
      auto mem = arenas.get_memory_handles();

      THEN("no memory allocation was collapsed") {
        CHECK(mem[vbo]->size() == 400);
        CHECK(mem[ibo]->size() == 500);
        CHECK(mem[texture]->size() == 600);
      }

      THEN("each memory has requested type") {
        CHECK(mem[vbo]->type() == 100);
        CHECK(mem[ibo]->type() == 200);
        CHECK(mem[texture]->type() == 300);
      }
    }
  }

  GIVEN("mem requirements same for two buffer puproses") {
    vlk::detail::purpose_data<vk::MemoryRequirements> requierments{
        req4type(100), req4type(100), req4type(300)
    };

    WHEN("arena_pools created") {
      vlk::detail::arena_pools<typed_memory> arenas{
          arena_sizes, [&](auto p) { return requierments[p]; }, typed_memory::allocate
      };
      auto mem = arenas.get_memory_handles();

      THEN("memory allocation was same type are collapsed") {
        CHECK(mem[vbo] == mem[ibo]);
        CHECK(mem[vbo]->size() == 900);
        CHECK(mem[vbo]->type() == 100);
      }

      THEN("memory allocation of different type is not collpsed") {
        CHECK(mem[texture] != mem[ibo]);
        CHECK(mem[texture]->type() == 300);
        CHECK(mem[texture]->size() == 600);
      }
    }
  }

  GIVEN("same mem requirements fow all purposes") {
    vlk::detail::purpose_data<vk::MemoryRequirements> requierments{
        req4type(100), req4type(100), req4type(100)
    };

    WHEN("arena_pools created") {
      vlk::detail::arena_pools<typed_memory> arenas{
          arena_sizes, [&](auto p) { return requierments[p]; }, typed_memory::allocate
      };
      auto mem = arenas.get_memory_handles();

      THEN("all memory allocation are collapsed") {
        CHECK(mem[vbo] == mem[ibo]);
        CHECK(mem[texture] == mem[ibo]);
        CHECK(mem[vbo]->size() == 1500);
        CHECK(mem[vbo]->type() == 100);
      }
    }
  }
}
