#include <array>
#include <span>

template <template <class...> class C, typename T, size_t N>
constexpr std::span<T> as_concated(C<std::array<T, N>> &chunked) noexcept {
  return std::span<T>{chunked.data()->data(), N * chunked.size()};
}

template <template <class...> class C, typename T, size_t N>
constexpr std::span<const T>
as_concated(const C<std::array<T, N>> &chunked) noexcept {
  return std::span<const T>{chunked.data()->data(), N * chunked.size()};
}

template <template <class...> class C, typename T, size_t N>
constexpr std::span<const T>
as_concated(const C<std::array<T, N>> &&chunked) = delete;

template <typename T, size_t N>
constexpr std::span<T>
as_concated(std::span<std::array<T, N>> chunked) noexcept {
  return std::span<T>{chunked.data()->data(), N * chunked.size()};
}

template <typename T, size_t N>
constexpr std::span<const T>
as_concated(std::span<const std::array<T, N>> chunked) noexcept {
  return std::span<const T>{chunked.data()->data(), N * chunked.size()};
}
