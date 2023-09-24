#pragma once

#include "IStreamable.h"
#include "SizeFinder.h"
#include "Stream.h"

namespace hbann
{
class StreamReader
{
  public:
    template <typename Type, typename... Types>
    static constexpr void ReadAll(const Stream &aStream, Type &aObject, Types &...aObjects)
    {
        using TypeRaw = get_raw_t<Type>;

        aObject = Read<TypeRaw>(aStream);

        if constexpr (sizeof...(aObjects))
        {
            ReadAll(aStream, aObjects...);
        }
    }

  private:
    template <typename Type> static [[nodiscard]] constexpr decltype(auto) Read(const Stream &aStream)
    {
        if constexpr (std::is_standard_layout_v<Type> && !std::is_pointer_v<Type>)
        {
            return ReadObjectOfKnownSize<Type>(aStream);
        }
        else if constexpr (std::is_base_of_v<IStreamable, Type>)
        {
            return ReadStreamable<Type>(aStream);
        }
        else if constexpr (std::ranges::range<Type>)
        {
            return ReadRange<Type>(aStream);
        }
        else
        {
            static_assert(always_false<Type>, "Type is not accepted!");
        }
    }

    template <typename Type> static [[nodiscard]] constexpr decltype(auto) ReadStreamable(Stream &aStream) noexcept
    {
        static_assert(std::is_base_of_v<IStreamable, Type>, "Type is not a streamable!");

        Type streamable{};
        streamable.FromStream(aStream.Read(ReadObjectOfKnownSize<size_range>()));
        return streamable;
    }

    template <typename Type> static [[nodiscard]] constexpr decltype(auto) ReadRange(const Stream &aStream)
    {
        static_assert(std::ranges::range<Type>, "Type is not a range!");

        Type range{};
        const auto size = ReadObjectOfKnownSize<size_range>();
        if constexpr (has_method_reserve_v<Type>)
        {
            range.reserve(size);
        }

        if constexpr (SizeFinder::FindRangeRank<Type>() > 1)
        {
            for (size_t i = 0; i < size; i++)
            {
                range.insert(std::ranges::cend(range), ReadRange<typename Type::value_type>(aStream));
            }
        }
        else
        {
            for (size_t i = 0; i < size; i++)
            {
                range.insert(std::ranges::cend(range), Read<typename Type::value_type>(aStream));
            }
        }

        return range;
    }

    template <typename Type>
    static [[nodiscard]] constexpr decltype(auto) ReadObjectOfKnownSize(Stream &aStream) noexcept
    {
        static_assert(std::is_standard_layout_v<Type> && !std::is_pointer_v<Type>,
                      "Type is not an object of known size or it is a pointer!");

        return *reinterpret_cast<const Type *>(aStream.Read(sizeof(Type)).data());
    }
};
} // namespace hbann
