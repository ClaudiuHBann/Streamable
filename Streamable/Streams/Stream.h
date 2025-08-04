/*
    Copyright (c) 2024 Claudiu HBann

    See LICENSE for the full terms of the MIT License.
*/

#pragma once

#include "StreamFile.h"

namespace hbann
{
// Represents a vector/file when used for serialization
// and a span/file when used for deserialization
class Stream
{
    friend class StreamReader;

    using vector = std::vector<uint8_t>;
    using span = std::span<const uint8_t>;
    using stream = std::variant<vector, span, StreamFile>;

  public:
    constexpr explicit Stream() noexcept : mStreamUsageType(StreamUsageType::SERIALIZE), mStream(vector())
    {
    }

    explicit Stream(const std::filesystem::path &aFile, const StreamUsageType aType)
        : mStreamUsageType(aType), mStream(StreamFile(aFile, aType))
    {
    }

    constexpr explicit Stream(const span aSpan) noexcept
        : mStreamUsageType(StreamUsageType::DESERIALIZE), mStream(aSpan)
    {
    }

    constexpr Stream(Stream &&aStream) noexcept
    {
        *this = std::move(aStream);
    }

    constexpr vector &&Release() noexcept
    {
        assert(!IsView());
        if (IsStreamFile())
        {
            return (std::move)(GetStreamFile().Release());
        }
        else
        {
            return (std::move)(GetStream());
        }
    }

    template <typename FunctionSeek>
    constexpr decltype(auto) Peek(FunctionSeek &&aFunctionSeek, const Size::size_max aOffset = 0)
    {
        if (IsStreamFile())
        {
            GetStreamFile().Peek(std::move(aFunctionSeek), aOffset);
        }
        else
        {
            const auto readIndex = mReadIndex;
            mReadIndex += aOffset;
            aFunctionSeek(readIndex);
            mReadIndex = readIndex;
        }

        return *this;
    }

    constexpr decltype(auto) Reserve(const Size::size_max aSize)
    {
        assert(!IsView());
        assert(mStreamUsageType == StreamUsageType::SERIALIZE);

        if (IsStreamFile())
        {
            GetStreamFile().Reserve(aSize);
        }
        else
        {
            GetStream().reserve(aSize);
        }

        return *this;
    }

    [[nodiscard]] constexpr auto View() noexcept
    {
        assert(!IsStreamFile());
        return IsView() ? GetSpan() : GetStream();
    }

    [[nodiscard]] constexpr auto Read(Size::size_max aSize) noexcept
    {
        if (IsStreamFile())
        {
            return GetStreamFile().Read(aSize);
        }
        else
        {
            const auto view = View();

            // clamp read count
            if (mReadIndex + aSize > view.size())
            {
                aSize = view.size() - mReadIndex;
            }

            mReadIndex += aSize;
            return span{view.data() + (mReadIndex - aSize), aSize};
        }
    }

    [[nodiscard]] constexpr auto Current() noexcept
    {
        if (IsStreamFile())
        {
            return GetStreamFile().Current();
        }
        else
        {
            return View()[mReadIndex];
        }
    }

    constexpr auto IsEOS() noexcept
    {
        if (IsStreamFile())
        {
            return GetStreamFile().IsEOS();
        }
        else
        {
            return View().size() == mReadIndex;
        }
    }

    constexpr decltype(auto) Write(const span aSpan)
    {
        assert(!IsView());
        assert(mStreamUsageType == StreamUsageType::SERIALIZE);

        if (IsStreamFile())
        {
            GetStreamFile().Write(aSpan);
        }
        else
        {
            GetStream().insert(GetStream().end(), aSpan.data(), aSpan.data() + aSpan.size());
        }

        return *this;
    }

    constexpr Stream &operator=(Stream &&aStream) noexcept
    {
        mStreamUsageType = aStream.mStreamUsageType;
        mStream = std::move(aStream.mStream);
        mReadIndex = aStream.mReadIndex;

        return *this;
    }

    constexpr decltype(auto) Clear(const bool aRemoveFile = false) noexcept
    {
        if (IsView())
        {
            GetSpan() = {};
        }
        else if (IsStreamFile())
        {
            GetStreamFile().Clear(aRemoveFile);
        }
        else if (IsStream())
        {
            GetStream().clear();
            GetStream().shrink_to_fit();
        }

        mReadIndex = {};

        return *this;
    }

  private:
    StreamUsageType mStreamUsageType;

    stream mStream;
    Size::size_max mReadIndex{};

    constexpr bool IsView() const noexcept
    {
        return std::holds_alternative<span>(mStream);
    }

    constexpr bool IsStream() const noexcept
    {
        return std::holds_alternative<vector>(mStream);
    }

    constexpr bool IsStreamFile() const noexcept
    {
        return std::holds_alternative<StreamFile>(mStream);
    }

    constexpr vector &GetStream() noexcept
    {
        assert(IsStream());
        return std::get<vector>(mStream);
    }

    constexpr span &GetSpan() noexcept
    {
        assert(IsView());
        return std::get<span>(mStream);
    }

    constexpr StreamFile &GetStreamFile() noexcept
    {
        assert(IsStreamFile());
        return std::get<StreamFile>(mStream);
    }
};
} // namespace hbann
