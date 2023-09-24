#pragma once

#include "IStreamable.h"
#include "SizeFinder.h"
#include "Stream.h"

namespace hbann
{
class StreamReader
{
  public:
    constexpr StreamReader(Stream &aStream) noexcept : mStream(aStream)
    {
    }

    template <typename Type, typename... Types> constexpr void ReadAll(Type &aObject, Types &...aObjects)
    {
        using TypeRaw = get_raw_t<Type>;

        aObject = Read<TypeRaw>();

        if constexpr (sizeof...(aObjects))
        {
            ReadAll(aObjects...);
        }
    }

  private:
    Stream &mStream;

    template <typename Type> [[nodiscard]] constexpr decltype(auto) Read()
    {
        if constexpr (std::is_standard_layout_v<Type> && !std::is_pointer_v<Type>)
        {
            return ReadObjectOfKnownSize<Type>();
        }
        else if constexpr (std::is_base_of_v<IStreamable, Type>)
        {
            return ReadStreamable<Type>();
        }
        else if constexpr (std::ranges::range<Type>)
        {
            return ReadRange<Type>();
        }
        else
        {
            static_assert(always_false<Type>, "Type is not accepted!");
        }
    }

    template <typename Type> [[nodiscard]] constexpr decltype(auto) ReadStreamable() noexcept
    {
        static_assert(std::is_base_of_v<IStreamable, Type>, "Type is not a streamable!");

        Type streamable{};
        streamable.FromStream(mStream.Read(ReadObjectOfKnownSize<size_range>()));
        return streamable;
    }

    template <typename Type> [[nodiscard]] constexpr decltype(auto) ReadRange()
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
                range.insert(std::ranges::cend(range), ReadRange<typename Type::value_type>());
            }
        }
        else
        {
            for (size_t i = 0; i < size; i++)
            {
                range.insert(std::ranges::cend(range), Read<typename Type::value_type>());
            }
        }

        return range;
    }

    template <typename Type> [[nodiscard]] constexpr decltype(auto) ReadObjectOfKnownSize() noexcept
    {
        static_assert(std::is_standard_layout_v<Type> && !std::is_pointer_v<Type>,
                      "Type is not an object of known size or it is a pointer!");

        return *reinterpret_cast<const Type *>(mStream.Read(sizeof(Type)).data());
    }
};
} // namespace hbann
