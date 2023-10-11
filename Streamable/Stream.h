#pragma once

namespace hbann
{
class Stream
{
    friend class StreamReader;

    using vector = std::vector<char>;
    using span = std::span<const char>;
    using stream = std::variant<vector, span>;

  public:
    constexpr Stream() noexcept : mStream(vector())
    {
    }

    constexpr Stream(const span &aSpan) noexcept : mStream(aSpan)
    {
    }

    constexpr Stream &Reserve(const size_t aSize)
    {
        GetStream().reserve(aSize);
        return *this;
    }

    [[nodiscard]] constexpr span View() noexcept
    {
        const auto spen = std::get_if<span>(&mStream);
        return spen ? *spen : GetStream();
    }

    [[nodiscard]] constexpr span Read(size_t aSize) noexcept
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

    constexpr Stream &Write(const char *aData, const size_t aSize)
    {
        GetStream().insert(GetStream().end(), aData, aData + aSize);
        return *this;
    }

    constexpr Stream &operator=(Stream &&aStream) noexcept
    {
        mStream = std::move(aStream.mStream);
        mReadIndex = aStream.mReadIndex;

        return *this;
    }

    constexpr Stream &Clear() noexcept
    {
        GetStream().clear();
        return *this;
    }

  private:
    stream mStream{};
    size_t mReadIndex{};

    constexpr decltype(auto) GetStream() noexcept
    {
        // if crashed here --> it's read only (span)
        return std::get<vector>(mStream);
    }

    constexpr decltype(auto) GetSpan() noexcept
    {
        return std::get<span>(mStream);
    }

    template <typename FunctionSeek> inline void Seek(const FunctionSeek &aFunctionSeek)
    {
        const auto readIndex = mReadIndex;
        aFunctionSeek(readIndex);
        mReadIndex = readIndex;
    }
};
} // namespace hbann
