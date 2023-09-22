#pragma once

#include "IStreamable.h"
#include "Stream.h"

namespace hbann
{
class StreamableSizeFinder
{
  public:
    template <typename Type, typename... Types>
    static [[nodiscard]] constexpr decltype(auto) FindParseSize(const Type &aObject, const Types &...aObjects) noexcept
    {
        return FindObjectSize(aObject) + FindParseSize(aObjects...);
    }

    template <typename Type> static [[nodiscard]] constexpr decltype(auto) FindObjectSize(const Type &aObject) noexcept
    {
        if constexpr (std::is_standard_layout_v<Type> && !std::is_pointer_v<Type>)
        {
            return sizeof(Type);
        }
        else if constexpr (std::is_base_of_v<IStreamable, Type>)
        {
            return static_cast<const IStreamable *>(&aObject)->GetObjectsSize();
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

    template <typename Type> static [[nodiscard]] constexpr size_t FindRangeRank() noexcept
    {
        if constexpr (std::ranges::range<Type>)
        {
            return 1 + FindRangeRank<typename Type::value_type>();
        }
        else
        {
            return 0;
        }
    }

    template <typename Type> static [[nodiscard]] constexpr decltype(auto) FindRangeSize(const Type &aObject) noexcept
    {
        if constexpr (FindRangeRank<Type>())
        {
            const auto size = std::ranges::size(aObject);
            if (size)
            {
                return sizeof(size) + size * FindRangeSize(*std::ranges::cbegin(aObject));
            }
            else
            {
                // TODO: do we really need to specify that there are 0 elements in the range?
                return sizeof(size);
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
