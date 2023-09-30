#pragma once

#include "StreamReader.h"
#include "StreamWriter.h"

#define STREAMABLE_DEFINE_FROM_STREAM(baseClass, ...)                                                                  \
  public:                                                                                                              \
    [[nodiscard]] ::hbann::Stream &&FromStream(::hbann::Stream &&aStream) override                                     \
    {                                                                                                                  \
        if constexpr (std::string_view(#baseClass) != STREAMABLE_INTERFACE_NAME)                                       \
        {                                                                                                              \
            aStream = std::move(baseClass::FromStream(std::move(aStream)));                                            \
        }                                                                                                              \
                                                                                                                       \
        SetStream(std::move(aStream));                                                                                 \
        mStreamReader.ReadAll(__VA_ARGS__);                                                                            \
                                                                                                                       \
        return Release();                                                                                              \
    }

#define STREAMABLE_DEFINE_TO_STREAM(baseClass, ...)                                                                    \
  public:                                                                                                              \
    [[nodiscard]] ::hbann::Stream &&ToStream() override                                                                \
    {                                                                                                                  \
        if constexpr (std::string_view(#baseClass) != STREAMABLE_INTERFACE_NAME)                                       \
        {                                                                                                              \
            SetStream(baseClass::ToStream());                                                                          \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            Reserve(FindParseSize());                                                                                  \
        }                                                                                                              \
                                                                                                                       \
        mStreamWriter.WriteAll(__VA_ARGS__);                                                                           \
                                                                                                                       \
        return Release();                                                                                              \
    }

#define STREAMABLE_DEFINE_FIND_PARSE_SIZE(baseClass, ...)                                                              \
  protected:                                                                                                           \
    [[nodiscard]] constexpr size_t FindParseSize() const noexcept override                                             \
    {                                                                                                                  \
        size_t size{};                                                                                                 \
        if constexpr (std::string_view(#baseClass) != STREAMABLE_INTERFACE_NAME)                                       \
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

  public:
    [[nodiscard]] virtual Stream &&ToStream() = 0;
    [[nodiscard]] virtual Stream &&FromStream(Stream &&aStream) = 0;

  protected:
    StreamWriter mStreamWriter;
    StreamReader mStreamReader;

    inline IStreamable() : mStreamWriter(mStream), mStreamReader(mStream)
    {
    }

    [[nodiscard]] virtual constexpr size_t FindParseSize() const noexcept = 0;

    [[nodiscard]] virtual inline IStreamable *FindDerivedStreamable(StreamReader &)
    {
        return nullptr;
    }

    [[nodiscard]] constexpr decltype(auto) Release()
    {
        return std::move(mStream);
    }

    inline Stream &SetStream(Stream &&aStream)
    {
        mStream = std::move(aStream);
        mStreamWriter = StreamWriter(mStream);
        mStreamReader = StreamReader(mStream);

        return mStream;
    }

    inline decltype(auto) Reserve(const size_t aSize)
    {
        return mStream.Reserve(aSize);
    }

    IStreamable(const IStreamable &aIStreamable) : mStreamWriter(mStream), mStreamReader(mStream)
    {
        *this = aIStreamable;
    }

    IStreamable(IStreamable &&aIStreamable) : mStreamWriter(mStream), mStreamReader(mStream)
    {
        *this = std::move(aIStreamable);
    }

    IStreamable &operator=(const IStreamable &)
    {
        return *this;
    }

    IStreamable &operator=(IStreamable &&)
    {
        return *this;
    }

  private:
    Stream mStream;
};
} // namespace hbann
