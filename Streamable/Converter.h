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
    [[nodiscard]] static inline std::string ToUTF8(const std::wstring &aString)
    {
        return mConverter.to_bytes(aString);
    }

    [[nodiscard]] static inline std::wstring FromUTF8(const std::span<const char> &aString)
    {
        return mConverter.from_bytes(aString.data(), aString.data() + aString.size());
    }

    [[nodiscard]] static inline auto FromUTF8(const std::string &aString)
    {
        return FromUTF8(std::span<const char>{aString});
    }

  private:
    static inline std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> mConverter{};
};
} // namespace hbann
