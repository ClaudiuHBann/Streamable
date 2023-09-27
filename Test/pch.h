#pragma once

#include "../pch.h" // Streamable pch

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

// Test
#include <list>
#include <vector>
