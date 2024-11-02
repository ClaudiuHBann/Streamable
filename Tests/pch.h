#pragma once

// Streamable
#ifdef _WIN32

#include "Streamable/pch.h"
#include "Streams/IStreamable.h"

#else

#include "Streamable.hpp"

#endif // _WIN32

// Shared
#define CATCH_AMALGAMATED_CUSTOM_MAIN

#include "Catch2-3.7.0/catch_amalgamated.hpp"

using namespace Catch;

// Test
#include <iostream>
#include <list>
