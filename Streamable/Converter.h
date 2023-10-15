/*
    Copyright (c) 2023 Claudiu HBann

    See LICENSE for the full terms of the MIT License.
*/

#pragma once

namespace hbann
{
class Converter
{
  public:
    [[nodiscard]] static inline std::string ToUTF8(const std::wstring &aString) noexcept
    {
        const auto requiredSize =
            WideCharToMultiByte(CP_UTF8, 0, aString.c_str(), (int)aString.size(), nullptr, 0, nullptr, nullptr);
        if (!requiredSize)
        {
            return {};
        }

        std::string stringUTF8(requiredSize, 0);
        if (!WideCharToMultiByte(CP_UTF8, 0, aString.c_str(), (int)aString.size(), stringUTF8.data(), requiredSize,
                                 nullptr, nullptr))
        {
            return {};
        }

        return stringUTF8;
    }

    [[nodiscard]] static inline std::wstring FromUTF8(const std::span<const char> &aStringUTF8) noexcept
    {
        const auto requiredSize =
            MultiByteToWideChar(CP_UTF8, 0, aStringUTF8.data(), (int)aStringUTF8.size(), nullptr, 0);
        if (!requiredSize)
        {
            return {};
        }

        std::wstring string(requiredSize, 0);
        if (!MultiByteToWideChar(CP_UTF8, 0, aStringUTF8.data(), (int)aStringUTF8.size(), string.data(), requiredSize))
        {
            return {};
        }

        return string;
    }

    [[nodiscard]] static inline auto FromUTF8(const std::string &aStringUTF8) noexcept
    {
        return FromUTF8(std::span<const char>{aStringUTF8});
    }
};
} // namespace hbann
