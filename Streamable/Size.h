/*
    Copyright (c) 2024 Claudiu HBann

    See LICENSE for the full terms of the MIT License.
*/

#pragma once

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
        const auto size = static_cast<uint8_t>(aSize);
        if constexpr (sizeof(size_max) == 4)
        {
            return static_cast<size_max>(size >> 6);
        }
        else
        {
            return static_cast<size_max>(size >> 5);
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
        if constexpr (sizeof(size_max) == 4)
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
        static uint8_t SIZE_AS_CHARS[SIZE_MAX_IN_BYTES]{};
        static auto SIZE = reinterpret_cast<size_max *>(SIZE_AS_CHARS);

        const auto requiredBytes = FindRequiredBytes(aSize);
        auto SIZE_AS_CHARS_START = SIZE_AS_CHARS + (SIZE_MAX_IN_BYTES - requiredBytes);

        // write the size itself
        *SIZE = ToBigEndian(aSize);
        // write the 3 bits representing the bytes required
        if constexpr (sizeof(size_max) == 4)
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

    [[nodiscard]] static inline auto MakeSize(const span &aSize) noexcept
    {
        static uint8_t SIZE_AS_CHARS[SIZE_MAX_IN_BYTES]{};
        static auto SIZE = reinterpret_cast<size_max *>(SIZE_AS_CHARS);

        // clear the last size
        *SIZE = 0;

        size_max requiredBytes{};
        const auto size = static_cast<uint8_t>(aSize.front());
        if constexpr (sizeof(size_max) == 4)
        {
            requiredBytes = static_cast<size_max>(size >> 6);
        }
        else
        {
            requiredBytes = static_cast<size_max>(size >> 5);
        }
        auto SIZE_AS_CHARS_START = SIZE_AS_CHARS + (SIZE_MAX_IN_BYTES - requiredBytes);

        // copy only the resizeable size
        std::memcpy(SIZE_AS_CHARS_START, aSize.data(), requiredBytes);
        // clear the required bytes
        if constexpr (sizeof(size_max) == 4)
        {
            *SIZE_AS_CHARS_START &= 0b00111111;
        }
        else
        {
            *SIZE_AS_CHARS_START &= 0b00011111;
        }

        return ToBigEndian(*SIZE);
    }

  private:
    static inline constexpr auto SIZE_MAX_IN_BYTES = sizeof(size_max);

    template <typename AF = bool> [[nodiscard]] static constexpr size_max ToBigEndian(const size_max aSize) noexcept
    {
        if constexpr (std::endian::native == std::endian::little)
        {
            if constexpr (sizeof(size_max) == 4)
            {
                return ((aSize >> 24) & 0x000000FF) | ((aSize >> 8) & 0x0000FF00) | ((aSize << 8) & 0x00FF0000) |
                       ((aSize << 24) & 0xFF000000);
            }
            else if constexpr (sizeof(size_max) == 8)
            {
                return ((aSize & 0xFF00000000000000) >> 56) | ((aSize & 0x00FF000000000000) >> 40) |
                       ((aSize & 0x0000FF0000000000) >> 24) | ((aSize & 0x000000FF00000000) >> 8) |
                       ((aSize & 0x00000000FF000000) << 8) | ((aSize & 0x0000000000FF0000) << 24) |
                       ((aSize & 0x000000000000FF00) << 40) | ((aSize & 0x00000000000000FF) << 56);
            }
            else
            {
                static_assert(always_false<AF>, "Unknown size!");
            }
        }
        else if constexpr (std::endian::native == std::endian::big)
        {
            return aSize;
        }
        else
        {
            static_assert(always_false<AF>, "Unknown endianness!");
        }
    }
};
} // namespace hbann
