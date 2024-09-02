/*
    Copyright (c) 2024 Claudiu HBann

    See LICENSE for the full terms of the MIT License.
*/

#pragma once

namespace hbann
{
template <typename Type> [[nodiscard]] static constexpr Type ByteSwap(const Type aSize) noexcept
{
    static_assert(always_false<Type>, "ByteSwap is not implemented for this type!");
    return {};
}

template <> [[nodiscard]] constexpr uint32_t ByteSwap(const uint32_t aSize) noexcept
{
    return ((aSize >> 24) & 0x000000FF) | ((aSize >> 8) & 0x0000FF00) | ((aSize << 8) & 0x00FF0000) |
           ((aSize << 24) & 0xFF000000);
}

template <> [[nodiscard]] constexpr uint64_t ByteSwap(const uint64_t aSize) noexcept
{
    return ((aSize & 0xFF00000000000000) >> 56) | ((aSize & 0x00FF000000000000) >> 40) |
           ((aSize & 0x0000FF0000000000) >> 24) | ((aSize & 0x000000FF00000000) >> 8) |
           ((aSize & 0x00000000FF000000) << 8) | ((aSize & 0x0000000000FF0000) << 24) |
           ((aSize & 0x000000000000FF00) << 40) | ((aSize & 0x00000000000000FF) << 56);
}

class Converter
{
  public:
    template <typename Type> [[nodiscard]] static constexpr auto Encode(const Type &aString)
    {
        static_assert(is_utf16string<Type>, "Type must be a UTF16 string!");

#ifdef _WIN32
        const auto requiredSize = WideCharToMultiByte(CP_UTF8, 0, reinterpret_cast<LPCWCH>(aString.data()),
                                                      static_cast<int>(aString.size()), nullptr, 0, nullptr, nullptr);
        std::string str(requiredSize, 0);

        WideCharToMultiByte(CP_UTF8, 0, reinterpret_cast<LPCWCH>(aString.data()), static_cast<int>(aString.size()),
                            str.data(), requiredSize, nullptr, nullptr);
        return str;
#else
        return std::string_view{reinterpret_cast<const std::string::value_type *>(aString.data()),
                                aString.size() * sizeof(typename Type::value_type)};
#endif
    }

    template <typename Type> [[nodiscard]] static constexpr auto Decode(const std::span<const uint8_t> aString)
    {
        static_assert(is_utf16string<Type>, "Type must be a UTF16 string!");

#ifdef _WIN32
        const auto requiredSize = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<LPCCH>(aString.data()),
                                                      static_cast<int>(aString.size()), nullptr, 0);
        Type str(requiredSize, 0);

        MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<LPCCH>(aString.data()), static_cast<int>(aString.size()),
                            str.data(), requiredSize);
        return str;
#else
        using TypeValueType = typename Type::value_type;
        return std::basic_string_view<TypeValueType>{reinterpret_cast<const TypeValueType *>(aString.data()),
                                                     aString.size() / sizeof(TypeValueType)};
#endif
    }
};
} // namespace hbann
