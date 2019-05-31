#pragma once

#include <ratio>

template <auto Unit, typename Rep, typename Per>
class unit;

template <auto Unit, typename Rep, std::intmax_t Num, std::intmax_t Denum>
class unit<Unit, Rep, std::ratio<Num, Denum>> {
public:
  using period = std::ratio<Num, Denum>;

  constexpr unit() noexcept = default;
  explicit constexpr unit(Rep val) noexcept(
      std::is_nothrow_copy_constructible_v<Rep>)
      : value_{val} {}

  template <typename Rep2, typename Per2>
  constexpr unit(unit<Unit, Rep2, Per2> other) noexcept
      : value_{other.count() * std::ratio_divide<Per2, period>::num /
               std::ratio_divide<Per2, period>::den} {}

  constexpr Rep count() const
      noexcept(std::is_nothrow_copy_constructible_v<Rep>) {
    return value_;
  }

private:
  Rep value_{};
};

template <auto Unit, typename Rep, typename Per>
constexpr bool operator==(
    unit<Unit, Rep, Per> l, unit<Unit, Rep, Per> r) noexcept {
  return l.count() == r.count();
}

template <auto Unit, typename Rep, typename Per>
constexpr bool operator!=(
    unit<Unit, Rep, Per> l, unit<Unit, Rep, Per> r) noexcept {
  return l.count() != r.count();
}

template <auto Unit, typename Rep, typename Per>
constexpr auto operator+(
    unit<Unit, Rep, Per> l, unit<Unit, Rep, Per> r) noexcept {
  return unit<Unit, Rep, Per>{l.count() + r.count()};
}

template <auto Unit, typename Rep, typename Per>
constexpr auto operator-(
    unit<Unit, Rep, Per> l, unit<Unit, Rep, Per> r) noexcept {
  return unit<Unit, Rep, Per>{l.count() - r.count()};
}

template <auto Unit, typename Rep, typename Per, typename Num>
constexpr auto operator*(Num l, unit<Unit, Rep, Per> r) noexcept {
  return unit<Unit, decltype(l * r.count()), Per>{l * r.count()};
}

template <auto Unit, typename Rep, typename Per, typename Num>
constexpr auto operator*(unit<Unit, Rep, Per> l, Num r) noexcept {
  return unit<Unit, decltype(l.count() * r), Per>{l.count() * r};
}

template <auto Unit, typename Rep, typename Per, typename Num>
constexpr auto operator/(unit<Unit, Rep, Per> l, Num r) noexcept {
  return unit<Unit, Rep, Per>{l.count() / r};
}

template <auto Unit, typename Rep, typename Per>
constexpr auto operator/(
    unit<Unit, Rep, Per> l, unit<Unit, Rep, Per> r) noexcept {
  return l.count() / r.count();
}

enum class units { length };

template <typename Rep, typename Per>
using length = unit<units::length, Rep, Per>;

using meters = length<float, std::ratio<1>>;
using centimeters = length<float, std::centi>;
using millimeters = length<float, std::milli>;
