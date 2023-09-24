#pragma once

namespace hbann
{
class Stream
{
  public:
    inline Stream() : mStreamO(&mStream)
    {
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
        if (mReadCount + aSize > (size_range)streamView.size())
        {
            aSize = (size_range)streamView.size() - mReadCount;
        }

        mReadCount += aSize;
        return std::string_view{streamView.data() + (mReadCount - aSize), aSize};
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

  private:
    std::stringbuf mStream;
    std::ostream mStreamO;

    size_range mReadCount{};
};
} // namespace hbann
