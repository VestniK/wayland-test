#include "monotonic_arena.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>

namespace {

struct counted {
  explicit counted(int val = 0) : val{val} { ++instances_count; }
  ~counted() noexcept { --instances_count; }

  counted(const counted& rhs) noexcept : val{rhs.val} { ++instances_count; }
  counted& operator=(const counted& rhs) noexcept {
    val = rhs.val;
    return *this;
  }

  counted(counted&&) noexcept = delete;
  counted& operator=(counted&&) noexcept = delete;

  int val;

  static uint32_t instances_count;
};
uint32_t counted::instances_count = 0;

} // namespace

SCENARIO("allocate from arena") {
  GIVEN("arena created from vector") {
    std::vector<std::byte> buf;
    buf.resize(100500);
    std::span storage_mem_area{buf};
    monotonic_arena arena{std::move(buf)};

    THEN("capacity is equal to initial vector size") {
      CHECK(arena.capacity() == 100500);
    }

    WHEN("some bytes are allocated") {
      auto bytes = arena.allocate(20);

      THEN("capacity decreases") { CHECK(arena.capacity() == 100500 - 20); }

      THEN("allocated memory is inside initial vector") {
        CHECK(static_cast<std::byte*>(bytes) >= storage_mem_area.data());
        CHECK(static_cast<std::byte*>(bytes) + 20 <=
              storage_mem_area.data() + storage_mem_area.size());
      }
    }

    WHEN("adjacent allocations require adding due to alignment requirements") {
      auto one_byte [[maybe_unused]] = arena.allocate(1);
      auto aligned_bytes = arena.allocate(8, 128);

      THEN("capacity delta contains padding") {
        CHECK(arena.capacity() < 100500 - (1 + 8));
      }

      THEN("alligned allocated bytes has requested alignment") {
        CHECK(reinterpret_cast<uintptr_t>(aligned_bytes) % 128 == 0);
      }
    }

    WHEN("class with nontrivial constructor/destructor allocated") {
      auto val = arena.allocate_unique<counted>(42);

      THEN("sideeffects of constructor are observable") {
        CHECK(counted::instances_count == 1);
      }

      THEN("sideeffects destructor is called on allocated object destruction") {
        val.reset();
        CHECK(counted::instances_count == 0);
      }
    }
  }
}
