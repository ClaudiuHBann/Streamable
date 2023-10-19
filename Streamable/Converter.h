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
    [[nodiscard]] static auto FindUTF8Size(const std::wstring &aString) noexcept
    {
        size_t size{};
        for (size_t i = 0; i < aString.size(); i++)
        {
            if (aString[i] <= 0x7F)
            {
                size++;
            }
            else if (aString[i] <= 0x7FF)
            {
                size += 2;
            }
            else if (aString[i] <= 0xFFFF)
            {
                size += 3;
            }
            else
            {
                size += 4;
            }
        }

        return size;
    }

    // TODO: delete me in the future if I am useless
    [[nodiscard]] static auto FindUTF16Size(const std::wstring &aString) noexcept
    {
        const auto string = std::span<const char>{reinterpret_cast<const char *>(aString.data()),
                                                  aString.size() * sizeof(std::wstring::value_type)};

        size_t size{};
        for (size_t i = 0; i < string.size(); i++)
        {
            if ((string[i] & 0xC0) == 0x80)
            {
                continue;
            }

            if (!(string[i] & 0x80) || (string[i] & 0xE0) == 0xC0 || (string[i] & 0xF0) == 0xE0)
            {
                size += 2;
            }
            else if ((string[i] & 0xF8) == 0xF0)
            {
                size += 4;
            }
        }

        return size / sizeof(std::wstring::value_type);
    }

    [[nodiscard]] static inline std::string ToUTF8(const std::wstring &aString)
    {
        return mConverter.to_bytes(aString);
    }

    [[nodiscard]] static inline std::wstring FromUTF8(const std::span<const char> &aString)
    {
        return mConverter.from_bytes(aString.data(), aString.data() + aString.size());
    }

  private:
    static inline std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> mConverter{};
};
} // namespace hbann
