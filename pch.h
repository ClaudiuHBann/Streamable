#pragma once

// std
#include <algorithm>
#include <sstream>

using size_range = uint32_t;

template <typename> constexpr auto always_false = false;

template <typename Type> using get_raw_t = std::remove_const_t<std::remove_reference_t<Type>>;
