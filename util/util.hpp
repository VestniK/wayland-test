#pragma once

#include <type_traits>

namespace ut {

template<typename E>
auto underlying_cast(E e) noexcept {return static_cast<std::underlying_type_t<E>>(e);}

}
