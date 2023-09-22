#pragma once

namespace hbann
{
class Stream
{
  public:
    Stream() : mStreamI(&mStream), mStreamO(&mStream)
    {
    }

    template <class Self> [[nodiscard]] constexpr auto &&GetBuffer(this Self &&aSelf)
    {
        return forward<Self>(aSelf).mStream;
    }

  private:
    std::stringbuf mStream;
    std::istream mStreamI;
    std::ostream mStreamO;

    decltype(auto) Reserve(std::streamsize aSize, char *aData = nullptr)
    {
        return mStream.pubsetbuf(aData, aSize);
    }

    auto Read(char *aData, const std::streamsize aSize)
    {
        return mStreamI.readsome(aData, aSize);
    }

    decltype(auto) Write(const char *aData, const std::streamsize aSize)
    {
        return mStreamO.write(aData, aSize);
    }

    decltype(auto) Flush()
    {
        return mStreamO.flush();
    }
};
} // namespace hbann
