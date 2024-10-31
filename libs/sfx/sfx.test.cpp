#include "sfx.hpp"

#include <ranges>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

#include <thinsys/io/input.hpp>

using Catch::Matchers::UnorderedRangeEquals;

using namespace std::literals;

SCENARIO("Open self executable as zip-archive") {
  GIVEN("slef binary opened as zip archive") {
    auto archive = sfx::archive::open_self();

    WHEN("list of files requested") {
      const auto& entries = archive.entries();

      THEN("attached file are enlisted") {
        CHECK_THAT(entries | std::views::keys,
            UnorderedRangeEquals(std::vector{"a.txt"sv, "b.txt"sv}));
      }

      THEN("sizes of attached files are correct") {
        CHECK_THAT(entries | std::views::values |
                       std::views::transform(&sfx::archive::entry::size),
            UnorderedRangeEquals(std::vector{12, 35}));
      }
    }

    WHEN("existing resource opened") {
      auto& entry = archive.entries().at("a.txt");
      auto& fd = archive.open("a.txt");

      THEN("it's content can be read properly") {
        std::string content;
        content.resize(entry.size);
        thinsys::io::read(fd, std::as_writable_bytes(std::span{content}));
        CHECK(content == "Hello world\n");
      }
    }
  }
}
