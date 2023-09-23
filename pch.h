#pragma once

// std
#include <algorithm>
#include <sstream>

using size_range = uint32_t;

template <typename> constexpr auto always_false = false;

template <typename Type> using get_raw_t = std::remove_const_t<std::remove_reference_t<Type>>;

// Streamable
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
