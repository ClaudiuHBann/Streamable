#pragma once

namespace hbann
{
#ifdef STREAMABLE_USE_RESIZEABLE_SIZE
/*
    Format: first 3 bits + the actual size at last

    The first 3 bits represent how many bytes are necessary to represent the actual size with those 3 bits and they are
   written at the most left side of the 1-8 bytes.

    The actual size is written to the left most side of the 1-8 bytes.
*/
class Size
{
  public:
    using size_max = uint64_t; // UINT64_MAX / 8
    using span = std::span<const char>;

    [[nodiscard]] static constexpr auto FindRequiredBytes(const char aSize) noexcept
    {
        return static_cast<uint8_t>(aSize >> 5);
    }

    [[nodiscard]] static inline auto MakeSize(const size_max aSize) noexcept
    {
        static char SIZE_AS_CHARS[SIZE_MAX_IN_BYTES]{};
        static auto SIZE = reinterpret_cast<size_max *>(SIZE_AS_CHARS);

        const auto requiredBytes = FindRequiredBytes(aSize);
        auto SIZE_AS_CHARS_START = SIZE_AS_CHARS + (SIZE_MAX_IN_BYTES - requiredBytes);

        // write the size itself
        *SIZE = ToBigEndian(aSize);
        // write the 3 bits representing the bytes required
        *SIZE_AS_CHARS_START |= requiredBytes << 5;

        // return the last 'requiredBytes' from SIZE
        return span{SIZE_AS_CHARS_START, requiredBytes};
    }

    [[nodiscard]] static inline auto MakeSize(const span &aSize) noexcept
    {
        static char SIZE_AS_CHARS[SIZE_MAX_IN_BYTES]{};
        static auto SIZE = reinterpret_cast<size_max *>(SIZE_AS_CHARS);

        // clear the last size
        *SIZE = 0;

        const uint8_t requiredBytes = aSize.front() >> 5;
        auto SIZE_AS_CHARS_START = SIZE_AS_CHARS + (SIZE_MAX_IN_BYTES - requiredBytes);

        // trim the garbage
        std::memcpy(SIZE_AS_CHARS_START, aSize.data(), requiredBytes);
        // clear the required bytes
        *SIZE_AS_CHARS_START &= 0b00011111;

        return ToBigEndian(*SIZE);
    }

  private:
    static constexpr auto SIZE_MAX_IN_BYTES = sizeof(size_max);

    [[nodiscard]] static constexpr uint8_t FindRequiredBytes(const size_max aSize) noexcept
    {
        uint8_t requiredBits{};
        if (aSize)
        {
            // add the bits required to represent the size
            requiredBits += static_cast<uint8_t>(std::log2(aSize)) + 1;
        }
        // add the bits required to represent the required bytes to store the final value
        requiredBits += 3;

        // add 7 bits to round the final value up
        return static_cast<uint8_t>((requiredBits + (SIZE_MAX_IN_BYTES - 1)) / SIZE_MAX_IN_BYTES);
    }

    template <typename AF = bool> [[nodiscard]] static constexpr size_max ToBigEndian(const size_max aSize) noexcept
    {
        if constexpr (std::endian::native == std::endian::little)
        {
            return ((aSize & 0xFF00000000000000) >> 56) | ((aSize & 0x00FF000000000000) >> 40) |
                   ((aSize & 0x0000FF0000000000) >> 24) | ((aSize & 0x000000FF00000000) >> 8) |
                   ((aSize & 0x00000000FF000000) << 8) | ((aSize & 0x0000000000FF0000) << 24) |
                   ((aSize & 0x000000000000FF00) << 40) | ((aSize & 0x00000000000000FF) << 56);
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
#else
class Size
{
  public:
    using size_max = uint32_t; // UINT32_MAX
    using span = std::span<const char>;

    [[nodiscard]] static constexpr auto FindRequiredBytes(const char) noexcept
    {
        return static_cast<uint8_t>(sizeof(size_max));
    }

    [[nodiscard]] static inline auto MakeSize(const size_max &aSize) noexcept
    {
        return span{reinterpret_cast<const char *>(&aSize), sizeof(size_max)};
    }

    [[nodiscard]] static inline auto MakeSize(const span &aSize) noexcept
    {
        return *reinterpret_cast<const size_max *>(aSize.data());
    }
};
#endif // STREAMABLE_USE_RESIZEABLE_SIZE
} // namespace hbann
