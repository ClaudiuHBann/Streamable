#pragma once

namespace hbann
{
class Stream
{
    friend class StreamReader;

  public:
    using vector = std::vector<char>;
    using span = std::span<const char>;
    using stream = std::variant<vector, span>;

    constexpr Stream() noexcept : mStream(vector())
    {
    }

    constexpr Stream(const span &aSpan) noexcept : mStream(aSpan)
    {
    }

    constexpr Stream(Stream &&aStream) noexcept
    {
        *this = std::move(aStream);
    }

    constexpr decltype(auto) Reserve(const uint64_t aSize)
    {
        GetStream().reserve(static_cast<size_t>(aSize));
        return *this;
    }

    [[nodiscard]] constexpr auto View() noexcept
    {
        const auto spen = std::get_if<span>(&mStream);
        return spen ? *spen : GetStream();
    }

    [[nodiscard]] constexpr auto Read(uint64_t aSize) noexcept
    {
        const auto view = View();

        // clamp read count
        if (mReadIndex + aSize > view.size())
        {
            aSize = view.size() - mReadIndex;
        }

        mReadIndex += aSize;
        return span{view.data() + (mReadIndex - aSize), static_cast<size_t>(aSize)};
    }

    [[nodiscard]] constexpr auto Current() noexcept
    {
        return View()[static_cast<size_t>(mReadIndex)];
    }

    constexpr decltype(auto) Write(const span &aSpan)
    {
        GetStream().insert(GetStream().end(), aSpan.data(), aSpan.data() + aSpan.size());
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
    uint64_t mReadIndex{};

    constexpr vector &GetStream() noexcept
    {
        // if crashed here --> it's read only (span)
        return std::get<vector>(mStream);
    }

    template <typename FunctionSeek> inline decltype(auto) Seek(const FunctionSeek &aFunctionSeek)
    {
        const auto readIndex = mReadIndex;
        aFunctionSeek(readIndex);
        mReadIndex = readIndex;

        return *this;
    }
};
} // namespace hbann
