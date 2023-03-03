#include <catch2/catch_test_macros.hpp>

#include <fmt/format.h>

#include <util/unit.hpp>

using namespace fmt::literals;

template <typename Per> struct prefix;

template <> struct prefix<std::ratio<1>> {
  static constexpr const char *value = "";
};

template <> struct prefix<std::centi> {
  static constexpr const char *value = "centi";
};

template <> struct prefix<std::milli> {
  static constexpr const char *value = "milli";
};

template <typename Per> constexpr const char *prefix_v = prefix<Per>::value;

namespace Catch {

template <typename Rep, typename Per>
struct StringMaker<unit<units::length, Rep, Per>> {
  static std::string convert(unit<units::length, Rep, Per> val) {
    return fmt::format("{}{}meters", val.count(), prefix_v<Per>);
  }
};

} // namespace Catch

TEST_CASE("unit conversions", "[unit]") {
  SECTION("meters are converted to centimeters") {
    centimeters len{meters{5}};
    REQUIRE(len.count() == 500);
  }
  SECTION("centimeters are converted to millimeters") {
    millimeters len{centimeters{7}};
    REQUIRE(len.count() == 70);
  }
}

TEST_CASE("unit to scalar multiplication", "[unit]") {
  SECTION("must return unit with proper value") {
    REQUIRE(0.5f * meters{14} == meters{7});
    REQUIRE(centimeters{6} * 2 == centimeters{12});
  }
}

template <typename L, typename R, typename = void>
struct can_multiply : std::false_type {};

template <typename L, typename R>
struct can_multiply<
    L, R, std::void_t<decltype(std::declval<L>() * std::declval<R>())>>
    : std::true_type {};

template <typename L, typename R>
constexpr bool can_multiply_v = can_multiply<L, R>::value;

static_assert(!can_multiply_v<meters, meters>);
static_assert(!can_multiply_v<meters, centimeters>);
static_assert(can_multiply_v<float, meters>);
static_assert(can_multiply_v<meters, int>);

template <typename L, typename R, typename = void>
struct can_divide : std::false_type {};

template <typename L, typename R>
struct can_divide<L, R,
                  std::void_t<decltype(std::declval<L>() / std::declval<R>())>>
    : std::true_type {};

template <typename L, typename R>
constexpr bool can_divide_v = can_divide<L, R>::value;

static_assert(can_divide_v<millimeters, float>);
static_assert(can_divide_v<millimeters, int>);
static_assert(can_divide_v<millimeters, millimeters>);
