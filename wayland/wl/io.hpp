#pragma once

#include <iostream>

#include <util/util.hpp>

#include "util.hpp"

namespace wl {

inline std::ostream& operator<< (std::ostream& out, version ver) {
  return out << ut::underlying_cast(ver);
}
inline std::ostream& operator<< (std::ostream& out, serial val) {
  const auto flags = out.flags();
  out << std::hex << ut::underlying_cast(val);
  out.setf(flags);
  return out;
}
inline std::ostream& operator<< (std::ostream& out, keymap_format val) {
  switch (val) {
  case keymap_format::no_keymap: return out << "keymap_format::no_keymap";
  case keymap_format::xkb_v1: return out << "keymap_format::xkb_v1";
  }
  return out << "keymap_format::unknown[" << ut::underlying_cast(val) << "]";
}

inline std::ostream& operator<< (std::ostream& out, key_state val) {
  switch (val) {
  case key_state::pressed: return out << "key_state::pressed";
  case key_state::released: return out << "key_state::released";
  }
  return out << "key_state::unknown[" << ut::underlying_cast(val) << "]";
}

}
