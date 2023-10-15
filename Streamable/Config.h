/*
    Copyright (c) 2023 Claudiu HBann

    See LICENSE for the full terms of the MIT License.
*/

#pragma once

namespace hbann
{
class Config
{
  public:
    enum class Flags : uint8_t
    {
        RESIZEABLE_SIZE = 1 << 0,
        UTF8_STRINGS = 1 << 1,
    };

    enum class Type : uint8_t
    {
        PERFORMANCE,
        MEMORY
    };

    constexpr bool Get(const Flags aFlag) noexcept
    {
        return mFlags & static_cast<decltype(mFlags)>(aFlag);
    }

    constexpr void Add(const Flags aFlag) noexcept
    {
        mFlags |= static_cast<decltype(mFlags)>(aFlag);
    }

    constexpr void Remove(const Flags aFlag) noexcept
    {
        mFlags &= ~static_cast<decltype(mFlags)>(aFlag);
    }

    constexpr auto Reset(const Type aType) noexcept
    {
        switch (aType)
        {
        case Type::PERFORMANCE:
            return (mFlags = 0);

        case Type::MEMORY:
            return (mFlags = static_cast<decltype(mFlags)>(Flags::RESIZEABLE_SIZE) |
                             static_cast<decltype(mFlags)>(Flags::UTF8_STRINGS));

        default:
            return static_cast<decltype(mFlags)>(0);
        }
    }

  private:
    uint8_t mFlags{};
};
} // namespace hbann
