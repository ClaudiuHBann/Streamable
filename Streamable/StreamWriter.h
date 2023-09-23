#pragma once

#include "IStreamable.h"
#include "SizeFinder.h"
#include "Stream.h"

namespace hbann
{
class StreamWriter
{
  public:
    template <typename Type> static inline void WriteObjectOfKnownSize(Stream &aStream, const Type &aObject)
    {
        using TypeRaw = get_raw_t<Type>;

        static_assert(std::is_standard_layout_v<TypeRaw> && !std::is_pointer_v<TypeRaw>,
                      "Type is not an object of known size or it is a pointer!");

        const auto objectPtr = reinterpret_cast<const char *>(&aObject);
        aStream.Write(objectPtr, sizeof(aObject));
    }

    static inline void WriteStreamable(Stream &aStream, IStreamable &aStreamable)
    {
        const auto streamableSize = (size_range)aStreamable.FindParseSize();
        WriteObjectOfKnownSize(aStream, streamableSize);

        const auto &streamableParsedBuffer = aStreamable.ToStream().GetBuffer();
        aStream.Write(streamableParsedBuffer.view().data(), streamableSize);
    }

    template <typename Type> static constexpr void WriteRange(Stream &aStream, const Type &aRange)
    {
        using TypeRaw = get_raw_t<Type>;

        static_assert(std::ranges::range<TypeRaw>, "Type is not a range!");

        WriteObjectOfKnownSize((size_range)std::ranges::size(aRange));

        if constexpr (SizeFinder::FindRangeRank<TypeRaw>() > 1)
        {
            std::ranges::for_each(aRange, [&](const auto &aObject) { WriteRange(aStream, aObject); });
        }
        else
        {
            std::ranges::for_each(aRange, [&](const auto &aObject) { Write(aStream, aObject); });
        }
    }

    template <typename Type> static constexpr void Write(Stream &aStream, const Type &aObject)
    {
        using TypeRaw = get_raw_t<Type>;

        if constexpr (std::is_standard_layout_v<TypeRaw> && !std::is_pointer_v<TypeRaw>)
        {
            WriteObjectOfKnownSize(aStream, aObject);
        }
        else if constexpr (std::is_base_of_v<IStreamable, TypeRaw>)
        {
            WriteStreamable(aStream, aObject);
        }
        else if constexpr (std::ranges::range<TypeRaw>)
        {
            WriteRange(aStream, aObject);
        }
        else
        {
            static_assert(always_false<TypeRaw>, "Type is not accepted!");
        }
    }
};
} // namespace hbann
