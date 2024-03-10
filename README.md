
# Streamable

Fastest and Smallest (De)Serializer for C++20 or newer.

## Table of Contents

- [Installation](#installation)
- [Features](#features)
- [Usage](#usage)
- [Downsides and Limitations](#Downsides-and-Limitations)
- [Examples](#examples)

## Installation

To use this library, simply download and include the header file `Streamable.hpp` into your project or use NuGet to install it.

## Features

- **fast** - fast++ and memory-- compared to [MsgPack](https://msgpack.org/)
- **easy-to-use** - inherit a class and use a macro :D
- **single-header** - just copy paste the file into your project
- **has no dependencies** - it only uses the `C++20` standard library
- **supports multiple compilers** - `MSVC` and `GCC`
- **supports every data type** - beside **itself** (so called "streamables"), **raw/smart pointers** (ex:. `std::unique_ptr`, `std::shared_ptr` etc...), **most STL classes** (`std::tuple`, `std::optional`, `std::variant` etc...), **any nested range** (ex.: `std::wstring`, `std::list`, std::vector&lt;std::list&gt; etc...), **PODs** (ex.: POD structs and classes, enums, etc...), **primitive types** (ex.: `bool`, `unsigned int`, `double` etc...)

## Usage

1. Inherit from the `IStreamable` class or any class that implements it.
2. Use the macro **STREAMABLE_DEFINE** where you need to pass the base class and the objects you want to parse
3. **OPTIONAL** If "streamables" pointers are (de)serialized you MUST implement **FindDerivedStreamable** (if you forget this, a `static_assert` will appear explaining what is wrong)

## Downsides and Limitations

While **Streamable** offers many benefits, it's essential to consider its limitations as well:
-  **Custom Format** - it has a custom but simple format for storing the data
- **Programming Language** - supports only C++ >= 20

## Examples

- [Examples](https://github.com/ClaudiuHBann/Streamable_v2/blob/master/Test/Main.cpp) - Mixed
