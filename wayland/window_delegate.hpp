#pragma once

#include <wayland/util/geom.hpp>

struct basic_delegate {
  virtual ~basic_delegate() noexcept = default;
  virtual void resize(size sz) = 0;
};
