#pragma once

#include <type_traits>

template<typename E>
class bitmask {
public:
  using value_type = std::underlying_type_t<E>;

  constexpr bitmask(const E e) noexcept: val(static_cast<value_type>(e)) {}
  constexpr bitmask() = default;

  constexpr bitmask operator|(const bitmask rhs) const noexcept {return bitmask(val | rhs.val);}
  bitmask& operator|=(const bitmask rhs) noexcept {val = static_cast<value_type>(val | rhs.val); return *this;}

  constexpr bitmask operator&(const bitmask rhs) const noexcept {return bitmask(val & rhs.val);}
  bitmask& operator&=(const bitmask rhs) noexcept {val = static_cast<value_type>(val & rhs.val); return *this;}

  constexpr bitmask operator^(const bitmask rhs) const noexcept {return bitmask(val ^ rhs.val);}
  bitmask& operator^=(const bitmask rhs) noexcept {val = static_cast<value_type>(val ^ rhs.val); return *this;}

  constexpr bitmask operator~() const noexcept {return bitmask{~val};}

  constexpr bool operator<(const bitmask rhs) const noexcept {return val < rhs.val;}
  constexpr bool operator<=(const bitmask rhs) const noexcept {return val <= rhs.val;}
  constexpr bool operator==(const bitmask rhs) const noexcept {return val == rhs.val;}
  constexpr bool operator!=(const bitmask rhs) const noexcept {return val != rhs.val;}
  constexpr bool operator>=(const bitmask rhs) const noexcept {return val >= rhs.val;}
  constexpr bool operator>(const bitmask rhs) const noexcept {return val > rhs.val;}

  constexpr explicit operator bool() const noexcept {return val != 0;}

  constexpr value_type value() const noexcept {return val;}

private:
  constexpr bitmask(const value_type t) noexcept : val(t) {}

private:
  value_type val = 0;
};
