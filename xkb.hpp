#pragma once

#include <memory>
#include <string>

#include <xkbcommon/xkbcommon.h>

namespace xkb {

struct deleter {
  void operator() (xkb_context* ptr) {xkb_context_unref(ptr);}
  void operator() (xkb_keymap* ptr) {xkb_keymap_unref(ptr);}
};

template<typename T>
using unique_ptr = std::unique_ptr<T, deleter>;

class keymap {
public:
  keymap() noexcept = default;
  keymap(unique_ptr<xkb_keymap> ptr): ptr_(std::move(ptr)) {}

  xkb_keycode_t key_by_name(const char* name) const {
    return xkb_keymap_key_by_name(ptr_.get(), name);
  }
  xkb_keycode_t key_by_name(const std::string& name) const {
    return key_by_name(name.c_str());
  }

  const char* key_get_name(xkb_keycode_t key) const {
    return xkb_keymap_key_get_name(ptr_.get(), key);
  }
private:
  unique_ptr<xkb_keymap> ptr_;
};

class context {
public:
  context(): ptr_(xkb_context_new(XKB_CONTEXT_NO_FLAGS)) {
    if (!ptr_)
      throw std::runtime_error{"failed to create XKB context"};
  }

  keymap load_keyap(const std::byte* data, size_t size) {
    return unique_ptr<xkb_keymap>{xkb_keymap_new_from_buffer(
      ptr_.get(), reinterpret_cast<const char*>(data), size, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS
    )};
  }

private:
  unique_ptr<xkb_context> ptr_;
};

}
