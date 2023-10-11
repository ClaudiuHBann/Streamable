#pragma once

#include "StreamReader.h"
#include "StreamWriter.h"

#define STREAMABLE_DEFINE_FROM_STREAM(baseClass, ...)                                                                  \
  public:                                                                                                              \
    [[nodiscard]] void FromStream() override                                                                           \
    {                                                                                                                  \
        if constexpr (!::hbann::static_equal(#baseClass, STREAMABLE_INTERFACE_NAME))                                   \
        {                                                                                                              \
            baseClass::FromStream();                                                                                   \
        }                                                                                                              \
                                                                                                                       \
        mStreamReader.ReadAll(__VA_ARGS__);                                                                            \
    }

#define STREAMABLE_DEFINE_TO_STREAM(baseClass, ...)                                                                    \
  public:                                                                                                              \
    [[nodiscard]] void ToStream() override                                                                             \
    {                                                                                                                  \
        if constexpr (!::hbann::static_equal(#baseClass, STREAMABLE_INTERFACE_NAME))                                   \
        {                                                                                                              \
            baseClass::ToStream();                                                                                     \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            Reserve(FindParseSize());                                                                                  \
        }                                                                                                              \
                                                                                                                       \
        mStreamWriter.WriteAll(__VA_ARGS__);                                                                           \
    }

#define STREAMABLE_DEFINE_FIND_PARSE_SIZE(baseClass, ...)                                                              \
  protected:                                                                                                           \
    [[nodiscard]] constexpr size_t FindParseSize() override                                                            \
    {                                                                                                                  \
        size_t size{};                                                                                                 \
        if constexpr (!::hbann::static_equal(#baseClass, STREAMABLE_INTERFACE_NAME))                                   \
        {                                                                                                              \
            size += baseClass::FindParseSize();                                                                        \
        }                                                                                                              \
                                                                                                                       \
        size += ::hbann::SizeFinder::FindParseSize(__VA_ARGS__);                                                       \
                                                                                                                       \
        return size;                                                                                                   \
    }

#define STREAMABLE_DEFINE_INTRUSIVE                                                                                    \
  private:                                                                                                             \
    friend class ::hbann::StreamReader;                                                                                \
    friend class ::hbann::StreamWriter;

#define STREAMABLE_DEFINE(baseClass, ...)                                                                              \
    STREAMABLE_DEFINE_INTRUSIVE                                                                                        \
    STREAMABLE_DEFINE_TO_STREAM(baseClass, __VA_ARGS__)                                                                \
    STREAMABLE_DEFINE_FROM_STREAM(baseClass, __VA_ARGS__)                                                              \
    STREAMABLE_DEFINE_FIND_PARSE_SIZE(baseClass, __VA_ARGS__)

namespace hbann
{
class IStreamable
{
    friend class SizeFinder;
    friend class StreamWriter;
    friend class StreamReader;

    Stream mStream;

  public:
    [[nodiscard]] inline decltype(auto) Serialize()
    {
        ToStream();
        return Release();
    }

    inline void Deserialize(Stream &&aStream, const bool aClear = true)
    {
        Swap(std::move(aStream));
        FromStream();

        if (aClear)
        {
            mStream.Clear();
        }
    }

  protected:
    StreamWriter mStreamWriter;
    StreamReader mStreamReader;

    constexpr IStreamable() noexcept : mStreamWriter(mStream), mStreamReader(mStream)
    {
    }

    virtual void ToStream() = 0;
    virtual void FromStream() = 0;

    [[nodiscard]] virtual size_t FindParseSize() = 0;

    [[nodiscard]] constexpr Stream &&Release() noexcept
    {
        return std::move(mStream);
    }

    constexpr Stream &Swap(Stream &&aStream) noexcept
    {
        mStream = std::move(aStream);
        mStreamWriter = StreamWriter(mStream);
        mStreamReader = StreamReader(mStream);

        return mStream;
    }

    constexpr decltype(auto) Reserve(const size_t aSize)
    {
        return mStream.Reserve(aSize);
    }

    constexpr IStreamable(const IStreamable &aIStreamable) noexcept : IStreamable()
    {
        *this = aIStreamable;
    }

    constexpr IStreamable(IStreamable &&aIStreamable) noexcept : IStreamable()
    {
        *this = std::move(aIStreamable);
    }

    constexpr IStreamable &operator=(const IStreamable &) noexcept
    {
        return *this;
    }

    constexpr IStreamable &operator=(IStreamable &&aIStreamable) noexcept
    {
        Swap(std::move(aIStreamable.mStream));
        return *this;
    }
};
} // namespace hbann
