#pragma once

// std
#include <cstdint>
#include <sstream>

// Streamable
#define STREAMABLE_INTERFACE_NAME "IStreamable"

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define DISCARD(value) [[maybe_unused]] auto CONCAT(_, __LINE__)(value)

namespace hbann
{
using size_range = uint32_t;

template <typename> constexpr auto always_false = false;

template <typename Type> using get_raw_t = std::remove_const_t<std::remove_reference_t<Type>>;

template <class Container>
concept has_method_reserve_v = requires(Container &aContainer) { aContainer.reserve; };
} // namespace hbann

/*
    TODO:
         - write the size of the data itself everytime not the size or the elements count
         - create our own exception class for more specific info
*/
