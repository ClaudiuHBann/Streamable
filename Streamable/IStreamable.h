/*
    Copyright (c) 2024 Claudiu HBann

    See LICENSE for the full terms of the MIT License.
*/

#pragma once

#include "StreamReader.h"
#include "StreamWriter.h"

namespace hbann
{
class IStreamable
{
    friend class StreamWriter;
    friend class StreamReader;

    Stream mStream;

  public:
    [[nodiscard]] inline decltype(auto) Serialize()
    {
        ToStream();
        return Release();
    }

    inline void Deserialize(Stream &&aStream, const bool aClear = true)
    {
        Swap(std::move(aStream));
        FromStream();

        if (aClear)
        {
            mStream.Clear();
        }
    }

  protected:
    StreamWriter mStreamWriter;
    StreamReader mStreamReader;

    constexpr IStreamable() noexcept : mStreamWriter(mStream), mStreamReader(mStream)
    {
    }

    virtual void ToStream() = 0;
    virtual void FromStream() = 0;

    [[nodiscard]] constexpr Stream &&Release() noexcept
    {
        return std::move(mStream);
    }

    constexpr Stream &Swap(Stream &&aStream) noexcept
    {
        mStream = std::move(aStream);
        mStreamWriter = StreamWriter(mStream);
        mStreamReader = StreamReader(mStream);

        return mStream;
    }

    constexpr decltype(auto) Reserve(const Size::size_max aSize)
    {
        return mStream.Reserve(aSize);
    }

    constexpr IStreamable(const IStreamable &aIStreamable) noexcept : IStreamable()
    {
        *this = aIStreamable;
    }

    constexpr IStreamable(IStreamable &&aIStreamable) noexcept : IStreamable()
    {
        *this = std::move(aIStreamable);
    }

    constexpr IStreamable &operator=(const IStreamable &) noexcept
    {
        return *this;
    }

    constexpr IStreamable &operator=(IStreamable &&aIStreamable) noexcept
    {
        Swap(std::move(aIStreamable.mStream));
        return *this;
    }
};
} // namespace hbann
