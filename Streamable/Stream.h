#pragma once

namespace hbann
{
class Stream
{
  public:
    inline Stream() : mStreamI(&mStream), mStreamO(&mStream)
    {
    }

    template <class Self> [[nodiscard]] constexpr auto &&GetBuffer(this Self &&aSelf)
    {
        return forward<Self>(aSelf).mStream;
    }

    inline decltype(auto) Reserve(std::streamsize aSize, char *aData = nullptr)
    {
        return mStream.pubsetbuf(aData, aSize);
    }

    inline decltype(auto) Read(char *aData, const std::streamsize aSize)
    {
        return mStreamI.read(aData, aSize);
    }

    inline decltype(auto) Write(const char *aData, const std::streamsize aSize)
    {
        return mStreamO.write(aData, aSize);
    }

    inline decltype(auto) Flush()
    {
        return mStreamO.flush();
    }

  private:
    std::basic_stringbuf<char, std::char_traits<char>, std::allocator<char>> mStream;
    std::istream mStreamI;
    std::ostream mStreamO;
};
} // namespace hbann
