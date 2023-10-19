/*
    Copyright (c) 2023 Claudiu HBann

    See LICENSE for the full terms of the MIT License.
*/

#pragma once

#include "Converter.h"
#include "Size.h"
#include "Stream.h"

namespace hbann
{
class IStreamable;

class SizeFinder
{
  public:
    template <typename Type, typename... Types>
    [[nodiscard]] static constexpr Size::size_max FindParseSize(Type &aObject, Types &...aObjects) noexcept
    {
        return FindObjectSize<std::remove_cvref_t<Type>>(aObject) + FindParseSize(aObjects...);
    }

    template <typename Type> [[nodiscard]] static constexpr Size::size_max FindRangeRank() noexcept
    {
        using TypeRaw = std::remove_cvref_t<Type>;

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
    [[nodiscard]] static constexpr Size::size_max GetRangeCount(const Range &aRange) noexcept
    {
        using RangeRaw = std::remove_cvref_t<Range>;

        if constexpr (has_method_size<RangeRaw>)
        {
            return std::ranges::size(aRange);
        }
        else if constexpr (is_path<RangeRaw>)
        {
            return aRange.native().size();
        }
        else
        {
            static_assert(always_false<RangeRaw>, "Tried to get the range count from an unknown object!");
        }
    }

  private:
    template <typename Type> [[nodiscard]] static constexpr Size::size_max FindObjectSize(Type &aObject) noexcept
    {
        if constexpr (std::ranges::range<Type>)
        {
            return FindRangeSize(aObject);
        }
        else if constexpr (is_base_of_no_ptr<IStreamable, Type>)
        {
            Size::size_max size{};
            if constexpr (is_pointer<Type>)
            {
                size = static_cast<IStreamable *>(aObject)->FindParseSize();
            }
            else
            {
                size = static_cast<IStreamable *>(&aObject)->FindParseSize();
            }

            return Size::FindRequiredBytes(size);
        }
        else if constexpr (is_std_lay_no_ptr<Type>)
        {
            return sizeof(Type);
        }
        else
        {
            static_assert(always_false<Type>, "Type is not accepted!");
        }
    }

    template <typename Type> [[nodiscard]] static constexpr Size::size_max FindRangeSize(Type &aRange) noexcept
    {
        Size::size_max size = Size::FindRequiredBytes(GetRangeCount(aRange));
        if constexpr (FindRangeRank<Type>() > 1)
        {
            for (auto &object : aRange)
            {
                size += FindRangeSize(object);
            }
        }
        else
        {
            size += FindRangeRank1Size(aRange);
        }

        return size;
    }

    template <typename Type> [[nodiscard]] static constexpr Size::size_max FindRangeRank1Size(Type &aRange) noexcept
    {
        static_assert(std::ranges::range<Type>, "Type is not a range!");

        using TypeValueType = typename Type::value_type;

        Size::size_max size{};
        if constexpr (is_range_std_lay<Type>)
        {
            if constexpr (std::is_same_v<Type, std::wstring>)
            {
                size += Converter::FindUTF8Size(aRange);
            }
            else
            {
                size += GetRangeCount(aRange) * sizeof(TypeValueType);
            }
        }
        else
        {
            for (auto &object : aRange)
            {
                size += FindObjectSize(object);
            }
        }

        return size;
    }

    static constexpr Size::size_max FindParseSize() noexcept
    {
        return 0;
    }
};
} // namespace hbann
