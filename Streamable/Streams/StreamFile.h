/*
    Copyright (c) 2024 Claudiu HBann

    See LICENSE for the full terms of the MIT License.
*/

#pragma once

#include "Utilities/Size.h"

namespace hbann
{
class StreamFile
{
    friend class Stream;

    using fstream = std::basic_fstream<uint8_t>;
    using vector = std::vector<uint8_t>;
    using span = std::span<const uint8_t>;

  public:
    // Only used by Stream's std::variant rules
    StreamFile(const StreamFile &)
    {
        assert(false);
    }

    explicit StreamFile(const std::filesystem::path &aPath, const StreamUsageType aStreamUsageType) : mPath(aPath)
    {
        switch (aStreamUsageType)
        {
            using enum StreamUsageType;

        case SERIALIZE:
            mStream = fstream(mPath, std::ios_base::out | std::ios_base::binary);
            break;

        case DESERIALIZE:
            mStream = fstream(mPath, std::ios_base::in | std::ios_base::binary);
            break;

        default:
            assert(false);
        }

        assert(mStream);
    }

    explicit StreamFile(StreamFile &&aStreamFile) noexcept
    {
        *this = std::move(aStreamFile);
    }

    StreamFile &operator=(StreamFile &&aStreamFile) noexcept
    {
        mPath = std::move(aStreamFile.mPath);
        mStream = std::move(aStreamFile.mStream);
        mBuffer = std::move(aStreamFile.mBuffer);
        mReadIndex = aStreamFile.mReadIndex;

        return *this;
    }

  private:
    vector &&Release() noexcept
    {
        return std::move(mBuffer);
    }

    template <typename FunctionSeek> StreamFile &Peek(FunctionSeek &&aFunctionSeek, const Size::size_max aOffset = 0)
    {
        const auto readIndex = mReadIndex;
        mReadIndex += aOffset;
        aFunctionSeek(readIndex);
        mReadIndex = readIndex;

        mStream.seekp(mReadIndex, std::ios_base::beg);

        return *this;
    }

    StreamFile &Reserve(const Size::size_max aSize)
    {
        mBuffer.reserve(aSize);

        return *this;
    }

    [[nodiscard]] span Read(Size::size_max aSize)
    {
        // clamp read count
        const auto fileSize = std::filesystem::file_size(mPath);
        if (mReadIndex + aSize > fileSize)
        {
            aSize = fileSize - mReadIndex;
        }

        // TODO: use mapped memory
        mBuffer.reserve(aSize);
        mStream.read(mBuffer.data(), aSize);

        mReadIndex += aSize;
        return mBuffer;
    }

    auto Current()
    {
        return static_cast<fstream::char_type>(mStream.peek());
    }

    bool IsEOS() const noexcept
    {
        return mStream.eof();
    }

    StreamFile &Write(const span aSpan)
    {
        mStream.write(aSpan.data(), aSpan.size());

        return *this;
    }

    StreamFile &Clear(const bool aRemoveFile = false)
    {
        mStream.close();

        if (aRemoveFile)
        {
            std::error_code ec;
            std::filesystem::remove(mPath);
        }

        mPath.clear();

        mBuffer.clear();
        mBuffer.shrink_to_fit();

        mReadIndex = {};

        return *this;
    }

  private:
    std::filesystem::path mPath;
    fstream mStream;
    vector mBuffer;
    Size::size_max mReadIndex{};
};
} // namespace hbann
