#include "pch.h"

// not in PCH because it's not used in the library itself
// and only in this single header maker
#include <fstream>

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace
{
constexpr auto LICENSE = R"(/*
    Copyright (c) 2024 Claudiu HBann
    
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/)"sv;

constexpr auto FILES = {R"(Utilities/Converter.h)"sv,  R"(Utilities/Size.h)"sv,       R"(Streams/Stream.h)"sv,
                        R"(Utilities/SizeFinder.h)"sv, R"(Streams/StreamReader.h)"sv, R"(Streams/StreamWriter.h)"sv};

constexpr auto FILE_FWD = R"(FWD/StreamableFWD.h)"sv;
constexpr auto FILE_PCH = R"(pch.h)"sv;
constexpr auto FILE_ISTREAMABLE = R"(Streams/IStreamable.h)"sv;

constexpr auto PATH_OUTPUT = R"(..\nuget\Streamable.hpp)"sv;

constexpr auto NS_START = "namespace hbann\n{"sv;
constexpr auto NS_END = R"(} // namespace hbann)"sv;
constexpr auto PRAGMA_ONCE = R"(#pragma once)"sv;
constexpr auto INCLUDE_STREAMABLE_FWD = R"(#include "FWD/StreamableFWD.h")"sv;
} // namespace

static auto ReadAllText(const std::filesystem::path &aPath)
{
    std::ifstream ifstream(aPath);
    if (!ifstream)
    {
        throw std::runtime_error(std::format("Could not open file '{}' !", aPath.string()));
    }

    std::ostringstream sstream;
    sstream << ifstream.rdbuf();
    return sstream.str();
}

static void WriteAllText(const std::filesystem::path &aPath, const std::string_view &aText)
{
    std::ofstream ofstream(aPath);
    if (!ofstream)
    {
        throw std::runtime_error(std::format("Could not open file '{}' !", aPath.string()));
    }

    ofstream << aText;
}

static auto ReadFWD()
{
    auto text = ReadAllText(FILE_FWD);
    text = text.substr(text.find(PRAGMA_ONCE) + PRAGMA_ONCE.size() + 2); // delete new lines too
    text.pop_back();                                                     // delete last new line
    return text;
}

static auto ReadPCH()
{
    auto text = ReadAllText(FILE_PCH);
    text = text.substr(text.find(PRAGMA_ONCE));
    text = text.substr(0, text.rfind(NS_END) - 1); // delete new line too

    auto start = text.begin() + text.find(INCLUDE_STREAMABLE_FWD);
    text = text.replace(start, start + INCLUDE_STREAMABLE_FWD.size(), ReadFWD());

    return text;
}

static auto ReadIStreamable()
{
    auto text = ReadAllText(FILE_ISTREAMABLE);
    text = text.substr(text.find(NS_START) + NS_START.size() + 1); // delete new line too
    return text;
}

int main()
{
    auto streamable = std::format("{}\n\n{}\n\n", LICENSE, ReadPCH());

    for (const auto &file : FILES)
    {
        auto text = ReadAllText(file);
        text = text.substr(text.find(NS_START) + NS_START.size() + 1); // delete new line too
        text = text.substr(0, text.rfind(NS_END));                     // keep new line too

        streamable += text + '\n';
    }

    streamable += ReadIStreamable();

    WriteAllText(PATH_OUTPUT, streamable);

    return 0;
}
