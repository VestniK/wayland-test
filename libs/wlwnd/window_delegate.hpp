#pragma once

#include <libs/geom/geom.hpp>

struct basic_delegate {
  virtual ~basic_delegate() noexcept = default;
  virtual void resize(size sz) = 0;
};
