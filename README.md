# Streamable

This library is all about speedy data parsing, churning out super small byte streams.

## Table of Contents

- [Installation](#installation)
- [Features](#features)
- [Usage](#usage)
- [Examples](#examples)
- [Documentation](#documentation)

## Installation

To use this library, simply get and include the header file `Streamable.hpp` into your project.

## Features

- **fast** - the parsing represents just a simple iteration, knows where every object is and how big it is, for example it reserves the memory for ranges that allow it before adding elements etc...
- **easy-to-use** - inherit a class and use a macro
- **single-header** - just copy paste the file in your project
- **simple format** - contains just the data itself and for the types that have a dynamic size a metadata representing just a uint32_t
- **has no dependencies** - uses just the standard library
- **accepts multiple data types** - beside **itself** (as a pointer or not) ofc, **primitive types** (ex.: bool, unsigned int, double etc...), **strings** (ex.: std::string. std::wstring etc...), **any type with standard layout** (ex.: POD structs and classes, enums, etc...), **nested ranges** (ex.: vector, list, vector&lt;list&gt; etc...)

## Usage

1. Inherit from the `IStreamable` class or any class that implements it.
2. Use the macro **STREAMABLE_DEFINE** where you need to pass the base class and the objects you want to parse
3. OPTIONAL If there are derived classes as base class pointers to parse you MUST implement **FindDerivedStreamable**

## Examples

- [Examples](https://github.com/ClaudiuHBann/Streamable_v2/blob/master/Test/Main.cpp) - Mixed

## Documentation

There are 5 macros:
- **STREAMABLE_DEFINE** - implements the necessary methods for parsing
- **STREAMABLE_DEFINE_INTRUSIVE** - there are some streamable methods that are not public and needs to be accessed
- **STREAMABLE_DEFINE_TO_STREAM** - implements the ToStream method
- **STREAMABLE_DEFINE_FROM_STREAM** - implements the FromStream method
- **STREAMABLE_DEFINE_FIND_PARSE_SIZE** - implements the FindParseSize method
