#pragma once

#include <concepts>
#include <expected>
#include <type_traits>

#include <catch2/matchers/catch_matchers_templated.hpp>

template <typename T>
class expected_matcher : public Catch::Matchers::MatcherGenericBase {
public:
  explicit expected_matcher(const T& val) : expectation_{val} {}

  template <typename U, typename E>
  bool match(const std::expected<U, E>& val) const {
    if (!val)
      return false;
    if constexpr (std::derived_from<T, Catch::Matchers::MatcherGenericBase>)
      return expectation_.match(val.value());
    else
      return val.value() == expectation_;
  }

  std::string describe() const override {
    if constexpr (std::derived_from<T, Catch::Matchers::MatcherGenericBase>)
      return "Contains expected value that: " + expectation_.describe();
    else
      return "Contains expected value equals: " + Catch::Detail::stringify(expectation_);
  }

private:
  T expectation_;
};

template <typename M>
auto expected_matches(const M& matcher) {
  return expected_matcher<M>(matcher);
}

template <typename T>
auto is_expected(const T& val) {
  return expected_matcher<std::decay_t<T>>(val);
}

auto is_expected(const char* val) { return expected_matcher<const char*>(val); }
