#pragma once

#include "Stream.h"

namespace hbann
{
class IStreamable;

class SizeFinder
{
  public:
    template <typename Type, typename... Types>
    [[nodiscard]] static constexpr size_t FindParseSize(const Type &aObject, const Types &...aObjects) noexcept
    {
        return FindObjectSize<get_raw_t<Type>>(aObject) + FindParseSize(aObjects...);
    }

    template <typename Type> [[nodiscard]] static constexpr size_t FindRangeRank() noexcept
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

    template <std::ranges::range Range>
    [[nodiscard]] static constexpr size_t GetRangeCount(const Range &aRange) noexcept
    {
        if constexpr (has_method_size<Range>)
        {
            return std::ranges::size(aRange);
        }
        else if constexpr (is_path<Range>)
        {
            return aRange.native().size();
        }
        else
        {
            static_assert(always_false<Range>, "Implement your own size getter bitch, sorry :(");
        }
    }

  private:
    template <typename Type> [[nodiscard]] static constexpr size_t FindObjectSize(const Type &aObject) noexcept
    {
        if constexpr (is_std_lay_no_ptr<Type>)
        {
            return sizeof(Type);
        }
        else if constexpr (is_base_of_no_ptr<IStreamable, Type>)
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

    template <typename Type> [[nodiscard]] static constexpr size_t FindRangeSize(const Type &aRange) noexcept
    {
        using TypeValueType = typename Type::value_type;

        size_t size{};
        if constexpr (FindRangeRank<Type>() > 1)
        {
            size += sizeof(size_range);
            for (const auto &object : aRange)
            {
                size += FindRangeSize(object);
            }
        }
        else
        {
            size += sizeof(size_range);
            if constexpr (is_range_std_lay<Type>)
            {
                size += GetRangeCount(aRange) * sizeof(TypeValueType);
            }
            else
            {
                for (const auto &object : aRange)
                {
                    size += FindObjectSize(object);
                }
            }
        }

        return size;
    }

    static constexpr size_t FindParseSize() noexcept
    {
        return 0;
    }
};
} // namespace hbann
