/*
    Copyright (c) 2024 Claudiu HBann

    See LICENSE for the full terms of the MIT License.
*/

#pragma once

#include "Converter.h"

namespace hbann
{
/*
    Format: first 3 bits + the actual size at last

    The first 3 bits represent how many bytes are necessary to represent the actual size with those 3 bits and they are
   written at the most left side of the 1-8 bytes.

    The actual size is written to the left most side of the 1-8 bytes.
*/
class Size
{
  public:
    using size_max = size_t; // size_t / (4/8)
    using span = std::span<const uint8_t>;

    [[nodiscard]] static constexpr auto FindRequiredBytes(const uint8_t aSize) noexcept
    {
        if constexpr (SIZE_MAX_IN_BYTES == 4)
        {
            return static_cast<size_max>(aSize >> 6);
        }
        else
        {
            return static_cast<size_max>(aSize >> 5);
        }
    }

    [[nodiscard]] static constexpr size_max FindRequiredBytes(const size_max aSize) noexcept
    {
        size_max requiredBits{1};
        if (aSize)
        {
            // add the bits required to represent the size
            requiredBits += static_cast<size_max>(std::log2(aSize));
        }

        // add the bits required to represent the required bytes to store the final value
        if constexpr (SIZE_MAX_IN_BYTES == 4)
        {
            requiredBits += 2;
        }
        else
        {
            requiredBits += 3;
        }

        // add 7 bits to round the final value up
        return (requiredBits + 7) / 8;
    }

    [[nodiscard]] static inline auto MakeSize(const size_max aSize) noexcept
    {
        thread_local uint8_t SIZE_AS_CHARS[SIZE_MAX_IN_BYTES]{};
        auto &SIZE = *reinterpret_cast<size_max *>(SIZE_AS_CHARS);

        const auto requiredBytes = FindRequiredBytes(aSize);
        auto SIZE_AS_CHARS_START = SIZE_AS_CHARS + (SIZE_MAX_IN_BYTES - requiredBytes);

        // write the size itself
        SIZE = ToBigEndian(aSize);
        // write the 3 bits representing the bytes required
        if constexpr (SIZE_MAX_IN_BYTES == 4)
        {
            *SIZE_AS_CHARS_START |= requiredBytes << 6;
        }
        else
        {
            *SIZE_AS_CHARS_START |= requiredBytes << 5;
        }

        // return the last 'requiredBytes' from SIZE
        return span{SIZE_AS_CHARS_START, requiredBytes};
    }

    [[nodiscard]] static inline auto MakeSize(const span aSize) noexcept
    {
        uint8_t SIZE_AS_CHARS[SIZE_MAX_IN_BYTES]{};
        auto &SIZE = *reinterpret_cast<size_max *>(SIZE_AS_CHARS);

        size_max requiredBytes{};
        if constexpr (SIZE_MAX_IN_BYTES == 4)
        {
            requiredBytes = static_cast<size_max>(aSize.front() >> 6);
        }
        else
        {
            requiredBytes = static_cast<size_max>(aSize.front() >> 5);
        }

        auto SIZE_AS_CHARS_START = SIZE_AS_CHARS + (SIZE_MAX_IN_BYTES - requiredBytes);
        // copy only the resizeable size
        std::memcpy(SIZE_AS_CHARS_START, aSize.data(), requiredBytes);

        // clear the required bytes
        if constexpr (SIZE_MAX_IN_BYTES == 4)
        {
            *SIZE_AS_CHARS_START &= 0b00111111;
        }
        else
        {
            *SIZE_AS_CHARS_START &= 0b00011111;
        }

        return ToBigEndian(SIZE);
    }

  private:
    static inline constexpr auto SIZE_MAX_IN_BYTES = sizeof(size_max);

    [[nodiscard]] static constexpr size_max ToBigEndian(const size_max aSize) noexcept
    {
        if constexpr (std::endian::native == std::endian::little)
        {
            return ByteSwap(aSize);
        }
        else
        {
            return aSize;
        }
    }
};
} // namespace hbann
