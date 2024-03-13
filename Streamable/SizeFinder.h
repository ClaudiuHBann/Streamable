/*
    Copyright (c) 2024 Claudiu HBann

    See LICENSE for the full terms of the MIT License.
*/

#pragma once

#include "Size.h"

namespace hbann
{
class SizeFinder
{
  public:
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
};
} // namespace hbann
