#pragma once

#include <compare>
#include <concepts>
#include <string>

#include <catch2/matchers/catch_matchers_templated.hpp>

#include <fmt/format.h>

template <typename E>
concept ordering_category = std::same_as<E, std::strong_ordering> ||
                            std::same_as<E, std::weak_ordering> ||
                            std::same_as<E, std::partial_ordering>;

template <ordering_category Cat, const Cat&... Ops>
struct format_str;

template <ordering_category Cat, const Cat&... Ops>
constexpr const char* format_str_v = format_str<Cat, Ops...>::value;

// strong_ordering
template <>
struct format_str<std::strong_ordering, std::strong_ordering::less> {
  static constexpr const char* value = "Less than value: {}";
};

template <>
struct format_str<std::strong_ordering, std::strong_ordering::less,
    std::strong_ordering::equal> {
  static constexpr const char* value = "Less than or equal to value: {}";
};

template <>
struct format_str<std::strong_ordering, std::strong_ordering::greater> {
  static constexpr const char* value = "Greater than value: {}";
};

template <>
struct format_str<std::strong_ordering, std::strong_ordering::greater,
    std::strong_ordering::equal> {
  static constexpr const char* value = "Greater than or equal to value: {}";
};

template <>
struct format_str<std::strong_ordering, std::strong_ordering::equal> {
  static constexpr const char* value = "Equal to value: {}";
};

// partial_ ordering
template <>
struct format_str<std::partial_ordering, std::partial_ordering::less> {
  static constexpr const char* value = "Less than value: {}";
};

template <>
struct format_str<std::partial_ordering, std::partial_ordering::less,
    std::partial_ordering::equivalent> {
  static constexpr const char* value = "Less than or equivalent to value: {}";
};

template <>
struct format_str<std::partial_ordering, std::partial_ordering::greater> {
  static constexpr const char* value = "Greater than value: {}";
};

template <>
struct format_str<std::partial_ordering, std::partial_ordering::greater,
    std::partial_ordering::equivalent> {
  static constexpr const char* value = "Greater than or eqivalent to value: {}";
};

template <>
struct format_str<std::partial_ordering, std::partial_ordering::equivalent> {
  static constexpr const char* value = "Equivalent to value: {}";
};

template <>
struct format_str<std::partial_ordering, std::partial_ordering::unordered> {
  static constexpr const char* value = "Unrdered with value: {}";
};

// weak_ordering
template <>
struct format_str<std::weak_ordering, std::weak_ordering::less> {
  static constexpr const char* value = "Less than value: {}";
};

template <>
struct format_str<std::weak_ordering, std::weak_ordering::less,
    std::weak_ordering::equivalent> {
  static constexpr const char* value = "Less than or equivalent to value: {}";
};

template <>
struct format_str<std::weak_ordering, std::weak_ordering::greater> {
  static constexpr const char* value = "Greater than value: {}";
};

template <>
struct format_str<std::weak_ordering, std::weak_ordering::greater,
    std::weak_ordering::equivalent> {
  static constexpr const char* value =
      "Greater than or equivalent to value: {}";
};

template <>
struct format_str<std::weak_ordering, std::weak_ordering::equivalent> {
  static constexpr const char* value = "Equivalent to value: {}";
};

// Matchers

template <ordering_category Cat, std::three_way_comparable<Cat> T,
    const Cat&... Co>
class order_matcher : public Catch::Matchers::MatcherGenericBase {
public:
  explicit order_matcher(const T& val) : expectation_{val} {}

  bool match(const T& val) const {
    return (((val <=> expectation_) == Co) || ...);
  }

  std::string describe() const override {
    return fmt::format(
        format_str_v<Cat, Co...>, Catch::Detail::stringify(expectation_));
  }

private:
  T expectation_;
};

template <std::three_way_comparable<std::strong_ordering> T>
auto gt(const T& val) {
  return order_matcher<std::strong_ordering, std::decay_t<T>,
      std::strong_ordering::greater>(val);
}

template <std::three_way_comparable<std::strong_ordering> T>
auto ge(const T& val) {
  return order_matcher<std::strong_ordering, std::decay_t<T>,
      std::strong_ordering::greater, std::strong_ordering::equal>(val);
}

template <std::three_way_comparable<std::strong_ordering> T>
auto lt(const T& val) {
  return order_matcher<std::strong_ordering, std::decay_t<T>,
      std::strong_ordering::less>(val);
}

template <std::three_way_comparable<std::strong_ordering> T>
auto le(const T& val) {
  return order_matcher<std::strong_ordering, std::decay_t<T>,
      std::strong_ordering::less, std::strong_ordering::equal>(val);
}

template <std::three_way_comparable<std::strong_ordering> T>
auto eq(const T& val) {
  return order_matcher<std::strong_ordering, std::decay_t<T>,
      std::strong_ordering::equal>(val);
}
