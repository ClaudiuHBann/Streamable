#pragma once

#include "StringBuffer.h"

namespace hbann
{
class Stream
{
    friend class StreamReader;

  public:
    inline Stream() : mStream(StringBuffer::State::BOTH), mStreamO(&mStream)
    {
    }

    // mStream.mState and mStreamO will be refreshed
    inline Stream(std::string_view aStringView) : mStream(StringBuffer::State::READ), mStreamO(&mStream)
    {
        PubSetBuf(const_cast<char *>(aStringView.data()), aStringView.size());
        mStreamO = std::move(OStream(&mStream));
    }

    // mStreamO will be refreshed
    inline Stream(Stream &&aStream) noexcept : mStreamO(&mStream)
    {
        *this = std::move(aStream);
    }

    constexpr const StringBuffer &GetBuffer() const noexcept
    {
        return mStream;
    }

    inline decltype(auto) Reserve(const size_t aSize)
    {
        mStream.pubsetbuf(nullptr, aSize);
        return *this;
    }

    [[nodiscard]] inline auto Read(size_t aSize)
    {
        ThrowIfCant(StringBuffer::State::READ);

        // clamp read count
        const auto streamView = mStream.view();
        if (mReadIndex + aSize > streamView.size())
        {
            aSize = streamView.size() - mReadIndex;
        }

        mReadIndex += aSize;
        return std::string_view{streamView.data() + (mReadIndex - aSize), aSize};
    }

    inline decltype(auto) Write(const char *aData, const size_t aSize)
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
        mStreamO = std::move(aStream.mStreamO);
        mReadIndex = aStream.mReadIndex;

        return *this;
    }

    inline decltype(auto) Clear() noexcept
    {
        mStream.Clear();
        return *this;
    }

  private:
    StringBuffer mStream;

    class OStream : public std::ostream
    {
      public:
        OStream(std::stringbuf *aStringBuf) : std::ostream(aStringBuf)
        {
        }

        OStream &operator=(OStream &&aOStream) noexcept
        {
            swap(aOStream);
            return *this;
        }
    } mStreamO;

    size_t mReadIndex{};

    inline decltype(auto) PubSetBuf(char *aData, const size_t aSize)
    {
        mStream.pubsetbuf(aData, aSize);
        return *this;
    }

    inline void ThrowIfCant(const StringBuffer::State aState) const
    {
        if (!mStream.Can(aState))
        {
            throw std::runtime_error("The stream cannot " + std::to_string(aState) + "!");
        }
    }

    template <typename FunctionSeek> inline void Seek(const FunctionSeek &aFunctionSeek)
    {
        const auto readIndex = mReadIndex;
        aFunctionSeek(readIndex);
        mReadIndex = readIndex;
    }
};
} // namespace hbann
