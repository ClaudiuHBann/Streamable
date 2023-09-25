#pragma once

#include "StreamReader.h"
#include "StreamWriter.h"

#define STREAMABLE_DEFINE_FROM_STREAM(baseClass, ...)                                                                  \
  public:                                                                                                              \
    [[nodiscard]] Stream &&FromStream(Stream &&aStream) override                                                       \
    {                                                                                                                  \
        if constexpr (#baseClass##s != STREAMABLE_INTERFACE_NAME)                                                      \
        {                                                                                                              \
            aStream = move(baseClass::FromStream(move(aStream)));                                                      \
        }                                                                                                              \
                                                                                                                       \
        SetStream(move(aStream));                                                                                      \
        mStreamReader.ReadAll(__VA_ARGS__);                                                                            \
                                                                                                                       \
        return Release();                                                                                              \
    }

#define STREAMABLE_DEFINE_TO_STREAM(baseClass, ...)                                                                    \
  public:                                                                                                              \
    [[nodiscard]] Stream &&ToStream() override                                                                         \
    {                                                                                                                  \
        if constexpr (#baseClass##s != STREAMABLE_INTERFACE_NAME)                                                      \
        {                                                                                                              \
            SetStream(baseClass::ToStream());                                                                          \
        }                                                                                                              \
                                                                                                                       \
        mStreamWriter.WriteAll(__VA_ARGS__);                                                                           \
                                                                                                                       \
        return Release();                                                                                              \
    }

#define STREAMABLE_DEFINE_FIND_PARSE_SIZE(baseClass, ...)                                                              \
  protected:                                                                                                           \
    constexpr [[nodiscard]] size_range FindParseSize() const noexcept override                                         \
    {                                                                                                                  \
        size_t size{};                                                                                                 \
        if constexpr (#baseClass##s != STREAMABLE_INTERFACE_NAME)                                                      \
        {                                                                                                              \
            size += baseClass::FindParseSize();                                                                        \
        }                                                                                                              \
                                                                                                                       \
        size += hbann::SizeFinder::FindParseSize(__VA_ARGS__);                                                         \
                                                                                                                       \
        return (size_range)size;                                                                                       \
    }

#define STREAMABLE_DEFINE(baseClass, ...)                                                                              \
    STREAMABLE_DEFINE_TO_STREAM(baseClass, __VA_ARGS__)                                                                \
    STREAMABLE_DEFINE_FROM_STREAM(baseClass, __VA_ARGS__)                                                              \
    STREAMABLE_DEFINE_FIND_PARSE_SIZE(baseClass, __VA_ARGS__)

namespace hbann
{
class IStreamable
{
    friend class StreamWriter;

  public:
    virtual [[nodiscard]] Stream &&ToStream() = 0;
    virtual [[nodiscard]] Stream &&FromStream(Stream &&aStream) = 0;

  protected:
    StreamWriter mStreamWriter;
    StreamReader mStreamReader;

    inline IStreamable() : mStreamWriter(mStream), mStreamReader(mStream)
    {
    }

    virtual [[nodiscard]] constexpr size_range FindParseSize() const noexcept = 0;

    virtual [[nodiscard]] inline IStreamable *FindDerivedStreamable()
    {
        return nullptr;
    }

    constexpr Stream &&Release()
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

  private:
    Stream mStream;
};
} // namespace hbann
