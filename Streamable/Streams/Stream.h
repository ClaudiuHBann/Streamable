/*
    Copyright (c) 2024 Claudiu HBann

    See LICENSE for the full terms of the MIT License.
*/

#pragma once

#include "Utilities/Size.h"

namespace hbann
{
class Stream
{
    friend class StreamReader;

  public:
    using vector = std::vector<uint8_t>;
    using span = std::span<const uint8_t>;
    using stream = std::variant<vector, span>;

    constexpr Stream() noexcept : mStream(vector())
    {
    }

    constexpr Stream(const Stream &) noexcept
    {
    }

    constexpr explicit Stream(const span aSpan) noexcept : mStream(aSpan)
    {
    }

    constexpr Stream(Stream &&aStream) noexcept
    {
        *this = std::move(aStream);
    }

    constexpr vector &&Release() noexcept
    {
        return std::move(GetStream());
    }

    template <typename FunctionSeek>
    constexpr decltype(auto) Peek(FunctionSeek &&aFunctionSeek, const Size::size_max aOffset = 0)
    {
        ThrowIfCantRead(aOffset);

        const auto readIndex = mReadIndex;
        mReadIndex += aOffset;
        aFunctionSeek(readIndex);
        mReadIndex = readIndex;

        return *this;
    }

    constexpr decltype(auto) Reserve(const Size::size_max aSize)
    {
        GetStream().reserve(aSize);
        return *this;
    }

    [[nodiscard]] constexpr auto View() noexcept
    {
        const auto spen = std::get_if<span>(&mStream);
        return spen ? *spen : GetStream();
    }

    [[nodiscard]] constexpr auto CanRead(const Size::size_max aSize) noexcept
    {
        const auto view = View();
        return mReadIndex + aSize <= view.size();
    }

    [[nodiscard]] constexpr auto Read(const Size::size_max aSize)
    {
        ThrowIfCantRead(aSize);

        mReadIndex += aSize;
        return span{View().data() + (mReadIndex - aSize), aSize};
    }

    [[nodiscard]] constexpr auto Current()
    {
        ThrowIfCantRead(1);
        return View()[mReadIndex];
    }

    constexpr decltype(auto) Write(const span aSpan)
    {
        GetStream().insert(GetStream().end(), aSpan.data(), aSpan.data() + aSpan.size());
        return *this;
    }

    constexpr Stream &operator=(const Stream &) noexcept
    {
        return *this;
    }

    constexpr Stream &operator=(Stream &&aStream) noexcept
    {
        mStream = std::move(aStream.mStream);
        mReadIndex = aStream.mReadIndex;

        return *this;
    }

    constexpr decltype(auto) Clear() noexcept
    {
        GetStream().clear();
        return *this;
    }

  private:
    stream mStream{};
    Size::size_max mReadIndex{};

    constexpr vector &GetStream() noexcept
    {
        // if crashed here --> it's read only (span)
        return std::get<vector>(mStream);
    }

    constexpr void ThrowIfCantRead(const Size::size_max aSize)
    {
        if (!CanRead(aSize))
        {
            throw std::out_of_range("Invalid Stream subscript!");
        }
    }
};
} // namespace hbann
