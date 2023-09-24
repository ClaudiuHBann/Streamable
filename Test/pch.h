#pragma once

// std
#include <sstream>

// Streamable
namespace hbann
{
using size_range = uint32_t;

template <typename> constexpr auto always_false = false;

template <typename Type> using get_raw_t = std::remove_const_t<std::remove_reference_t<Type>>;

template <typename Container, typename = void> struct has_method_reserve : std::false_type
{
};

template <typename Container>
struct has_method_reserve<Container, std::void_t<decltype(std::declval<Container>().reserve(0))>> : std::true_type
{
};

template <typename Type> constexpr auto has_method_reserve_v = has_method_reserve<Type>::value;
} // namespace hbann

#include "Streamable/IStreamable.h"
#include "Streamable/SizeFinder.h"
#include "Streamable/Stream.h"
#include "Streamable/StreamReader.h"
#include "Streamable/StreamWriter.h"

using namespace hbann;

// Shared
#define CATCH_AMALGAMATED_CUSTOM_MAIN

#include "Shared/Catch2-3.4.0/catch_amalgamated.hpp"

using namespace Catch;
