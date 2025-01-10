#pragma once

#include <algorithm>
#include <compare>
#include <concepts>
#include <span>
#include <utility>

#include <GLES2/gl2.h>

namespace gl {

template <typename T>
concept resource_handle =
    std::regular<T> && std::same_as<typename T::native_handle_t, GLuint> && requires(const T& t) {
      { t.native_handle() } -> std::same_as<typename T::native_handle_t>;
      { static_cast<bool>(t) };
      { T::invalid } -> std::same_as<const T&>;
    };

template <typename T>
concept scalar_resource_handle = resource_handle<T> && requires(T t) {
  { T::free(t) };
};

template <typename T>
concept array_resource_handle = resource_handle<T> && requires(std::span<T> t) {
  { T::free(t) };
};

template <auto Type>
class basic_handle {
public:
  using native_handle_t = GLuint;

  constexpr basic_handle() noexcept = default;
  constexpr explicit basic_handle(native_handle_t handle) noexcept : handle_{handle} {}
  constexpr std::strong_ordering operator<=>(const basic_handle&) const noexcept = default;

  constexpr native_handle_t native_handle() const noexcept { return handle_; }
  constexpr explicit operator bool() const noexcept { return handle_ != invalid.handle_; }

  static void free(basic_handle res) noexcept = delete;
  static void free(std::span<basic_handle> res) noexcept = delete;
  static const basic_handle invalid;

private:
  GLuint handle_ = 0;
};
template <auto Type>
constexpr basic_handle<Type> basic_handle<Type>::invalid = {};

template <typename T>
class resource;

template <scalar_resource_handle T>
class resource<T> {
public:
  using native_handle_t = T::native_handle_t;

  constexpr resource() noexcept = default;
  constexpr resource(T handle) noexcept : handle_{handle} {}

  resource(const resource&) = delete;
  resource& operator=(const resource&) = delete;

  resource(resource&& rhs) noexcept : handle_(rhs.release()) {}
  resource& operator=(resource&& rhs) noexcept {
    if (auto old = std::exchange(handle_, rhs.release()))
      T::free(old);
    return *this;
  }

  ~resource() noexcept {
    if (handle_)
      T::free(handle_);
  }

  constexpr explicit operator bool() const noexcept { return static_cast<bool>(handle_); }
  constexpr T release() noexcept { return std::exchange(handle_, T::invalid); }
  constexpr T get() noexcept { return handle_; }
  constexpr native_handle_t native_handle() noexcept { return handle_.native_handle(); }

private:
  T handle_ = T::invalid;
};

template <array_resource_handle T, size_t N>
class resource<T[N]> {
public:
  constexpr resource() noexcept { handls_.fill(T::invalid); }
  constexpr resource(std::span<const T, N> handls) noexcept {
    assert(
        std::ranges::all_of(handls, [](const T& h) { return !h; }) ||
        std::ranges::none_of(handls, [](const T& h) { return !h; })
    );
    std::ranges::copy(handls, handls_.begin());
  }

  resource(const resource&) = delete;
  resource& operator=(const resource&) = delete;

  resource(resource&& rhs) noexcept {
    auto oit = handls_.begin();
    for (auto& handle : rhs.handls_)
      *(oit++) = std::exchange(handle, T::invalid);
  }
  resource& operator=(resource&& rhs) noexcept {
    if (handls_.front())
      T::free(handls_);
    auto oit = handls_.begin();
    for (auto& handle : rhs.handls_)
      *(oit++) = std::exchange(handle, T::invalid);
    return *this;
  }

  ~resource() noexcept {
    if (handls_.front())
      T::free(handls_);
  }

  constexpr const T& operator[](size_t idx) const noexcept { return handls_[idx]; }

private:
  std::array<T, N> handls_;
};

} // namespace gl
