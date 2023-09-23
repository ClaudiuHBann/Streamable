#pragma once

#include "IStreamable.h"
#include "Stream.h"

namespace hbann
{
class SizeFinder
{
  public:
    template <typename Type, typename... Types>
    static [[nodiscard]] constexpr decltype(auto) FindParseSize(const Type &aObject, const Types &...aObjects) noexcept
    {
        return FindObjectSize(aObject) + FindParseSize(aObjects...);
    }

    template <typename Type> static [[nodiscard]] constexpr decltype(auto) FindObjectSize(const Type &aObject) noexcept
    {
        using TypeRaw = get_raw_t<Type>;

        if constexpr (std::is_standard_layout_v<TypeRaw> && !std::is_pointer_v<TypeRaw>)
        {
            return sizeof(TypeRaw);
        }
        else if constexpr (std::is_base_of_v<IStreamable, TypeRaw>)
        {
            return static_cast<const IStreamable *>(&aObject)->GetObjectsSize();
        }
        else if constexpr (std::ranges::range<TypeRaw>)
        {
            return FindRangeSize(aObject);
        }
        else
        {
            static_assert(always_false<TypeRaw>, "Type is not accepted!");
        }
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

    template <typename Type> static [[nodiscard]] constexpr decltype(auto) FindRangeSize(const Type &aObject) noexcept
    {
        using TypeRaw = get_raw_t<Type>;

        if constexpr (FindRangeRank<TypeRaw>())
        {
            const auto size = std::ranges::size(aObject);
            if (size)
            {
                return sizeof(size_range) + size * FindRangeSize(*std::ranges::cbegin(aObject));
            }
            else
            {
                // TODO: do we really need to specify that there are 0 elements in the range?
                return sizeof(size_range);
            }
        }
        else
        {
            return FindObjectSize(aObject);
        }
    }

  private:
    static constexpr size_t FindParseSize() noexcept
    {
        return 0;
    }
};
} // namespace hbann
