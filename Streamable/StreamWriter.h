#pragma once

#include "IStreamable.h"
#include "SizeFinder.h"
#include "Stream.h"

namespace hbann
{
class StreamWriter
{
  public:
    template <typename Type, typename... Types>
    constexpr void WriteAll(Stream &aStream, const Type &aObject, const Types &...aObjects)
    {
        using TypeRaw = get_raw_t<Type>;

        Write<TypeRaw>(aStream, aObject);

        if constexpr (sizeof...(aObjects))
        {
            WriteAll(aStream, aObjects...);
        }
    }

  private:
    template <typename Type> static inline void WriteObjectOfKnownSize(Stream &aStream, const Type &aObject)
    {
        static_assert(std::is_standard_layout_v<Type> && !std::is_pointer_v<Type>,
                      "Type is not an object of known size or it is a pointer!");

        const auto objectPtr = reinterpret_cast<const char *>(&aObject);
        aStream.Write(objectPtr, sizeof(aObject));
    }

    static inline void WriteStreamable(Stream &aStream, IStreamable &aStreamable)
    {
        WriteObjectOfKnownSize(aStream, aStreamable.FindParseSize());

        const auto stream = aStreamable.ToStream();
        aStream.Write(stream.data(), stream.size());
    }

    template <typename Type> static constexpr void WriteRange(Stream &aStream, const Type &aRange)
    {
        static_assert(std::ranges::range<Type>, "Type is not a range!");

        WriteObjectOfKnownSize((size_range)std::ranges::size(aRange));

        if constexpr (SizeFinder::FindRangeRank<Type>() > 1)
        {
            for (const auto &aObject : aRange)
            {
                WriteRange(aStream, aObject);
            }
        }
        else
        {
            for (const auto &aObject : aRange)
            {
                Write(aStream, aObject);
            }
        }
    }

    template <typename Type> static constexpr void Write(Stream &aStream, const Type &aObject)
    {
        if constexpr (std::is_standard_layout_v<Type> && !std::is_pointer_v<Type>)
        {
            WriteObjectOfKnownSize(aStream, aObject);
        }
        else if constexpr (std::is_base_of_v<IStreamable, Type>)
        {
            WriteStreamable(aStream, aObject);
        }
        else if constexpr (std::ranges::range<Type>)
        {
            WriteRange(aStream, aObject);
        }
        else
        {
            static_assert(always_false<Type>, "Type is not accepted!");
        }
    }
};
} // namespace hbann
