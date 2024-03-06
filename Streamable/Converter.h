/*
    Copyright (c) 2024 Claudiu HBann

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

    [[nodiscard]] static auto FindUTF16Size(const std::span<const uint8_t> &aString) noexcept
    {
        size_t size{};

        for (size_t i = 0; i < aString.size(); i++)
        {
            if (!(aString[i] & 0x80))
            {
                size++;
            }
            else if ((aString[i] & 0xE0) == 0xC0)
            {
                size++;
                i++;
            }
            else if ((aString[i] & 0xF0) == 0xE0)
            {
                size += 2;
                i += 2;
            }
            else if ((aString[i] & 0xF8) == 0xF0)
            {
                size += 2;
                i += 3;
            }
        }

        return size;
    }

    [[nodiscard]] static inline std::string ToUTF8(const std::wstring &aString)
    {
        return mConverter.to_bytes(aString);
    }

    [[nodiscard]] static inline std::wstring FromUTF8(const std::span<const uint8_t> &aString)
    {
        auto aStringChar = reinterpret_cast<const char *>(aString.data());
        return mConverter.from_bytes(aStringChar, aStringChar + aString.size());
    }

  private:
    static inline std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> mConverter{};
};
} // namespace hbann
