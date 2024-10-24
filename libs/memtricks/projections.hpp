#pragma once

#include <concepts>
#include <string_view>

template <auto M>
struct as_string_view_t;

template <typename C, std::convertible_to<std::string_view> T, T C::*M>
struct as_string_view_t<M> {
  constexpr std::string_view operator()(const C& obj) const noexcept {
    return static_cast<std::string_view>(obj.*M);
  }
};

template <auto M>
constexpr inline struct as_string_view_t<M> as_string_view;
