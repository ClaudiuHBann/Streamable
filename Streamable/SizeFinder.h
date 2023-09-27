#pragma once

#include "Stream.h"

namespace hbann
{
class IStreamable;

class SizeFinder
{
  public:
    template <typename Type, typename... Types>
    static [[nodiscard]] constexpr auto FindParseSize(const Type &aObject, const Types &...aObjects) noexcept
    {
        return FindObjectSize<get_raw_t<Type>>(aObject) + FindParseSize(aObjects...);
    }

    template <typename Type> static [[nodiscard]] constexpr size_t FindRangeRank() noexcept
    {
        using TypeRaw = get_raw_t<Type>;

        if constexpr (std::ranges::range<TypeRaw>)
        {
            return 1 + FindRangeRank<typename TypeRaw::value_type>();
        }
        else
        {
            return 0;
        }
    }

  private:
    template <typename Type> static [[nodiscard]] constexpr auto FindObjectSize(const Type &aObject) noexcept
    {
        if constexpr (std::is_standard_layout_v<Type> && !std::is_pointer_v<Type>)
        {
            return sizeof(Type);
        }
        else if constexpr (std::is_base_of_v<IStreamable, std::remove_pointer_t<Type>>)
        {
            if constexpr (std::is_pointer_v<Type>)
            {
                return static_cast<const IStreamable *>(aObject)->FindParseSize();
            }
            else
            {
                return static_cast<const IStreamable *>(&aObject)->FindParseSize();
            }
        }
        else if constexpr (std::ranges::range<Type>)
        {
            return FindRangeSize(aObject);
        }
        else
        {
            static_assert(always_false<Type>, "Type is not accepted!");
        }
    }

    template <typename Type> static [[nodiscard]] constexpr auto FindRangeSize(const Type &aObject) noexcept
    {
        size_t size{};
        if constexpr (FindRangeRank<Type>())
        {
            size += sizeof(size_range);
            for (const auto &object : aObject)
            {
                size += FindRangeSize(aObject);
            }
        }
        else
        {
            size += FindObjectSize(aObject);
        }

        return size;
    }

    static constexpr size_t FindParseSize() noexcept
    {
        return 0;
    }
};
} // namespace hbann
