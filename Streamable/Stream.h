#pragma once

#include "StringBuffer.h"

namespace hbann
{
class Stream
{
    friend class StreamReader;

  public:
    inline Stream() : mStreamO(&mStream)
    {
    }

    inline Stream(std::string_view aStringView) : mStreamO(&mStream)
    {
        // TODO: we remove the const but we know what we are doing 💪, right?
        Reserve((size_range)aStringView.size(), (char *)aStringView.data());
        mStreamO.move(std::ostream(&mStream)); // refresh output stream
    }

    inline Stream(Stream &&aStream) noexcept : mStreamO(&mStream)
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

    inline decltype(auto) Read(size_range aSize)
    {
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
        mStreamO.write(aData, aSize);
        return *this;
    }

    inline decltype(auto) Flush()
    {
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
