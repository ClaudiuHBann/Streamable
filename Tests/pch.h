#pragma once

// Streamable
#include "Streamable.hpp"

// Shared
#define CATCH_AMALGAMATED_CUSTOM_MAIN

#include <Catch2-3.7.0/catch_amalgamated.hpp>

using namespace Catch;

// nlohmann JSON
#include <json-3.11.3/json.hpp>

using namespace nlohmann;

// msgpack
#define MSGPACK_NO_BOOST

#include <msgpack.hpp>

using namespace msgpack;

// Test
#include <iostream>
#include <list>
