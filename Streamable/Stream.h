#pragma once

#include "StringBuffer.h"

namespace hbann
{
class Stream
{
    // TODO: delete after deserialize IStreamable* right
    friend class StreamReader;

  public:
    inline Stream() : mStream(StringBuffer::State::BOTH), mStreamO(&mStream)
    {
    }

    // mStream.mState and mStreamO will be refreshed
    inline Stream(std::string_view aStringView) : mStream(StringBuffer::State::READ), mStreamO(&mStream)
    {
        Reserve((size_range)aStringView.size(), const_cast<char *>(aStringView.data()));
        mStreamO.move(std::ostream(&mStream));
    }

    // mStreamO will be refreshed
    inline Stream(Stream &&aStream) : mStreamO(&mStream)
    {
        *this = std::move(aStream);
    }

    template <class Self> [[nodiscard]] constexpr auto &&GetBuffer(this Self &&aSelf)
    {
        return std::forward<Self>(aSelf).mStream;
    }

    inline void Reserve(const size_range aSize, char *aData = nullptr)
    {
        mStream.pubsetbuf(aData, aSize);
    }

    inline auto Read(size_range aSize)
    {
        ThrowIfCant(StringBuffer::State::READ);

        // clamp read count
        const auto streamView = mStream.view();
        if (mReadIndex + aSize > (size_range)streamView.size())
        {
            aSize = (size_range)streamView.size() - mReadIndex;
        }

        mReadIndex += aSize;
        return std::string_view{streamView.data() + (mReadIndex - aSize), aSize};
    }

    inline decltype(auto) Write(const char *aData, const size_range aSize)
    {
        ThrowIfCant(StringBuffer::State::WRITE);

        mStreamO.write(aData, aSize);
        return *this;
    }

    inline decltype(auto) Flush()
    {
        ThrowIfCant(StringBuffer::State::WRITE);

        mStreamO.flush();
        return *this;
    }

    inline Stream &operator=(Stream &&aStream) noexcept
    {
        mStream = std::move(aStream.mStream);
        mStreamO.move(std::move(aStream.mStreamO));
        mReadIndex = aStream.mReadIndex;

        return *this;
    }

  private:
    StringBuffer mStream;
    std::ostream mStreamO;

    size_range mReadIndex{};

    inline void ThrowIfCant(const StringBuffer::State aState) const
    {
        if (!mStream.Can(aState))
        {
            throw std::runtime_error(std::format("The stream cannot {}!", std::to_string(aState)));
        }
    }

    // TODO: delete after deserialize IStreamable* right
    constexpr size_range GetReadIndex() const noexcept
    {
        return mReadIndex;
    }

    constexpr void SetReadIndex(const size_range aReadIndex) noexcept
    {
        mReadIndex = aReadIndex;
    }
};
} // namespace hbann
