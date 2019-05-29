#include <array>
#include <gsl/span>

template <template <class...> class C, typename T, size_t N>
constexpr gsl::span<T> as_concated(C<std::array<T, N>>& chunked) noexcept {
  return gsl::span<T>{chunked.data()->data(),
      gsl::narrow<typename gsl::span<T>::size_type>(N * chunked.size())};
}

template <template <class...> class C, typename T, size_t N>
constexpr gsl::span<const T> as_concated(
    const C<std::array<T, N>>& chunked) noexcept {
  return gsl::span<const T>{chunked.data()->data(),
      gsl::narrow<typename gsl::span<T>::size_type>(N * chunked.size())};
}

template <template <class...> class C, typename T, size_t N>
constexpr gsl::span<const T> as_concated(
    const C<std::array<T, N>>&& chunked) = delete;

template <typename T, size_t N>
constexpr gsl::span<T> as_concated(
    gsl::span<std::array<T, N>> chunked) noexcept {
  return gsl::span<T>{chunked.data()->data(),
      gsl::narrow<typename gsl::span<T>::size_type>(N * chunked.size())};
}

template <typename T, size_t N>
constexpr gsl::span<const T> as_concated(
    gsl::span<const std::array<T, N>> chunked) noexcept {
  return gsl::span<const T>{chunked.data()->data(),
      gsl::narrow<typename gsl::span<T>::size_type>(N * chunked.size())};
}
