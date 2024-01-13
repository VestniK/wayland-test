#pragma once

#include <concepts>
#include <ranges>
#include <unordered_set>

#include <catch2/matchers/catch_matchers_templated.hpp>

class unique_range_matcher : public Catch::Matchers::MatcherGenericBase {
public:
  template <std::ranges::input_range R>
  bool match(const R& val) const {
    using value_type = std::ranges::range_value_t<R>;
    std::unordered_set<value_type> dedup;
    for (const auto& item : val) {
      auto [_, inserded] = dedup.insert(item);
      if (!inserded)
        return false;
    }
    return true;
  }

  std::string describe() const override { return "All elements are unique"; }
};

auto are_unique() { return unique_range_matcher{}; }
