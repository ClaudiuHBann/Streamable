/*
    Copyright (c) 2024 Claudiu HBann
    
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#pragma once

namespace hbann
{
class IStreamable;
class StreamReader;
} // namespace hbann

// std
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include <bit>
#include <cmath>
#include <codecvt>
#include <cstring>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

// Streamable
constexpr auto STREAMABLE_INTERFACE_NAME = "IStreamable";

#define STREAMABLE_DEFINE_FROM_STREAM(baseClass, ...)                                                                  \
  protected:                                                                                                           \
    constexpr void FromStream() override                                                                               \
    {                                                                                                                  \
        if constexpr (!::hbann::static_equal(#baseClass, STREAMABLE_INTERFACE_NAME))                                   \
        {                                                                                                              \
            baseClass::FromStream();                                                                                   \
        }                                                                                                              \
                                                                                                                       \
        mStreamReader.ReadAll(__VA_ARGS__);                                                                            \
    }

#define STREAMABLE_DEFINE_TO_STREAM(baseClass, ...)                                                                    \
  protected:                                                                                                           \
    constexpr void ToStream() override                                                                                 \
    {                                                                                                                  \
        if constexpr (!::hbann::static_equal(#baseClass, STREAMABLE_INTERFACE_NAME))                                   \
        {                                                                                                              \
            baseClass::ToStream();                                                                                     \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            Reserve(FindParseSize());                                                                                  \
        }                                                                                                              \
                                                                                                                       \
        mStreamWriter.WriteAll(__VA_ARGS__);                                                                           \
    }

#define STREAMABLE_DEFINE_FIND_PARSE_SIZE(baseClass, ...)                                                              \
  protected:                                                                                                           \
    [[nodiscard]] constexpr ::hbann::Size::size_max FindParseSize() override                                           \
    {                                                                                                                  \
        ::hbann::Size::size_max size{};                                                                                \
        if constexpr (!::hbann::static_equal(#baseClass, STREAMABLE_INTERFACE_NAME))                                   \
        {                                                                                                              \
            size += baseClass::FindParseSize();                                                                        \
        }                                                                                                              \
                                                                                                                       \
        size += ::hbann::SizeFinder::FindParseSize(__VA_ARGS__);                                                       \
                                                                                                                       \
        return size;                                                                                                   \
    }

#define STREAMABLE_DEFINE_INTRUSIVE                                                                                    \
  private:                                                                                                             \
    friend class ::hbann::SizeFinder;                                                                                  \
    friend class ::hbann::StreamReader;                                                                                \
    friend class ::hbann::StreamWriter;

#define STATIC_ASSERT_HAS_ISTREAMABLE_BASE(baseClass)                                                                  \
    static_assert(std::derived_from<baseClass, ::hbann::IStreamable>, "The class must inherit a streamable!");

#define STREAMABLE_DEFINE(baseClass, ...)                                                                              \
    STATIC_ASSERT_HAS_ISTREAMABLE_BASE(baseClass)                                                                      \
    STREAMABLE_DEFINE_INTRUSIVE                                                                                        \
    STREAMABLE_DEFINE_TO_STREAM(baseClass, __VA_ARGS__)                                                                \
    STREAMABLE_DEFINE_FROM_STREAM(baseClass, __VA_ARGS__)                                                              \
    STREAMABLE_DEFINE_FIND_PARSE_SIZE(baseClass, __VA_ARGS__)

namespace hbann
{
namespace detail
{
template <typename> struct is_pair : std::false_type
{
};
template <typename TypeFirst, typename TypeSecond> struct is_pair<std::pair<TypeFirst, TypeSecond>> : std::true_type
{
};

template <typename> struct is_tuple : std::false_type
{
};
template <typename... Types> struct is_tuple<std::tuple<Types...>> : std::true_type
{
};

template <typename> struct is_variant : std::false_type
{
};
template <typename... Types> struct is_variant<std::variant<Types...>> : std::true_type
{
};

template <typename> struct is_unique_ptr : std::false_type
{
};
template <typename Type> struct is_unique_ptr<std::unique_ptr<Type>> : std::true_type
{
};

template <typename> struct is_optional : std::false_type
{
};
template <typename Type> struct is_optional<std::optional<Type>> : std::true_type
{
};

template <typename> struct is_shared_ptr : std::false_type
{
};
template <typename Type> struct is_shared_ptr<std::shared_ptr<Type>> : std::true_type
{
};

template <typename> struct is_basic_string : std::false_type
{
};
template <typename Type> struct is_basic_string<std::basic_string<Type>> : std::true_type
{
};
} // namespace detail

template <typename Type> constexpr bool is_pair_v = detail::is_pair<Type>::value;
template <typename Type> constexpr bool is_tuple_v = detail::is_tuple<Type>::value;
template <typename Type> constexpr bool is_variant_v = detail::is_variant<Type>::value;
template <typename Type> constexpr bool is_optional_v = detail::is_optional<Type>::value;
template <typename Type> constexpr bool is_unique_ptr_v = detail::is_unique_ptr<Type>::value;
template <typename Type> constexpr bool is_shared_ptr_v = detail::is_shared_ptr<Type>::value;
template <typename Type> constexpr bool is_basic_string_v = detail::is_basic_string<Type>::value;

template <typename> constexpr auto always_false = false;

template <typename Type>
concept is_wstring = std::is_same_v<typename Type::value_type, std::wstring::value_type> && is_basic_string_v<Type>;

template <typename Container>
concept has_method_reserve =
    std::ranges::contiguous_range<Container> && requires(Container &aContainer) { aContainer.reserve(size_t(0)); };

template <typename Type>
concept is_smart_pointer = is_shared_ptr_v<Type> || is_unique_ptr_v<Type>;

template <typename Type>
concept is_any_pointer = std::is_pointer_v<Type> || is_smart_pointer<Type>;

template <typename Derived, typename Base>
concept is_derived_from_pointer =
    std::is_pointer_v<Derived> && std::derived_from<std::remove_pointer_t<Derived>, Base> ||
    (is_smart_pointer<Derived> && std::derived_from<typename Derived::element_type, Base>);

template <typename Type>
concept is_standard_layout_no_pointer = std::is_standard_layout_v<Type> && !is_any_pointer<Type>;

template <typename Type>
concept is_path = std::is_same_v<Type, std::filesystem::path>;

template <typename Container>
concept is_range_standard_layout =
    (std::ranges::contiguous_range<Container> && is_standard_layout_no_pointer<typename Container::value_type>) ||
    is_path<Container>;

template <typename Container>
concept has_method_size = requires(Container &aContainer) { std::ranges::size(aContainer); };

[[nodiscard]] constexpr bool static_equal(const char *aString1, const char *aString2) noexcept
{
    return *aString1 == *aString2 && (!*aString1 || static_equal(aString1 + 1, aString2 + 1));
}

template <typename Type, std::size_t vIndex = 0>
[[nodiscard]] constexpr Type variant_from_index(const std::size_t aIndex)
{
    static_assert(is_variant_v<Type>, "Type is not a variant!");

    if constexpr (vIndex < std::variant_size_v<Type>)
    {
        return aIndex ? variant_from_index<Type, vIndex + 1>(aIndex - 1) : Type{std::in_place_index<vIndex>};
    }
    else
    {
        throw std::out_of_range("Out of bounds variant index!");
    }
}

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
    static constexpr auto SIZE_MAX_IN_BYTES = sizeof(size_max);

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

class Stream
{
    friend class StreamReader;

  public:
    using vector = std::vector<uint8_t>;
    using span = std::span<const uint8_t>;
    using stream = std::variant<vector, span>;

    constexpr Stream() noexcept : mStream(vector())
    {
    }

    constexpr Stream(const span &aSpan) noexcept : mStream(aSpan)
    {
    }

    constexpr Stream(Stream &&aStream) noexcept
    {
        *this = std::move(aStream);
    }

    constexpr vector &&Release() noexcept
    {
        return std::move(GetStream());
    }

    template <typename FunctionSeek>
    inline decltype(auto) Peek(const FunctionSeek &aFunctionSeek, const Size::size_max aOffset = 0)
    {
        const auto readIndex = mReadIndex;
        mReadIndex += aOffset;
        aFunctionSeek(readIndex);
        mReadIndex = readIndex;

        return *this;
    }

    constexpr decltype(auto) Reserve(const Size::size_max aSize)
    {
        GetStream().reserve(aSize);
        return *this;
    }

    [[nodiscard]] constexpr auto View() noexcept
    {
        const auto spen = std::get_if<span>(&mStream);
        return spen ? *spen : GetStream();
    }

    [[nodiscard]] constexpr auto Read(Size::size_max aSize) noexcept
    {
        const auto view = View();

        // clamp read count
        if (mReadIndex + aSize > view.size())
        {
            aSize = view.size() - mReadIndex;
        }

        mReadIndex += aSize;
        return span{view.data() + (mReadIndex - aSize), aSize};
    }

    [[nodiscard]] constexpr auto Current() noexcept
    {
        return View()[mReadIndex];
    }

    constexpr decltype(auto) Write(const span &aSpan)
    {
        GetStream().insert(GetStream().end(), aSpan.data(), aSpan.data() + aSpan.size());
        return *this;
    }

    constexpr Stream &operator=(Stream &&aStream) noexcept
    {
        mStream = std::move(aStream.mStream);
        mReadIndex = aStream.mReadIndex;

        return *this;
    }

    constexpr decltype(auto) Clear() noexcept
    {
        GetStream().clear();
        return *this;
    }

  private:
    stream mStream{};
    Size::size_max mReadIndex{};

    constexpr vector &GetStream() noexcept
    {
        // if crashed here --> it's read only (span)
        return std::get<vector>(mStream);
    }
};

class SizeFinder
{
  public:
    template <typename Type, typename... Types>
    [[nodiscard]] static constexpr Size::size_max FindParseSize(Type &aObject, Types &...aObjects) noexcept
    {
        return FindObjectSize<std::remove_cvref_t<Type>>(aObject) + FindParseSize(aObjects...);
    }

    static constexpr Size::size_max FindParseSize() noexcept
    {
        return 0;
    }

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

  private:
    template <typename Type> [[nodiscard]] static constexpr Size::size_max FindObjectSize(Type &aObject) noexcept
    {
        if constexpr (is_optional_v<Type>)
        {
            return aObject.has_value() ? FindObjectSize(*aObject) : 1;
        }
        else if constexpr (is_variant_v<Type>)
        {
            Size::size_max size{};
            std::visit([&](auto &&aArg) { size += FindObjectSize(aArg); }, aObject);
            return size;
        }
        else if constexpr (is_tuple_v<Type>)
        {
            Size::size_max size{};
            std::apply([&](auto &&...aArgs) { size += FindParseSize(aArgs...); }, aObject);
            return size;
        }
        else if constexpr (is_pair_v<Type>)
        {
            // we remove the constness of map's pair's key so we can use the already implemented FindParseSize branches
            auto &first = const_cast<std::remove_const_t<typename Type::first_type> &>(aObject.first);
            return FindParseSize(first, aObject.second);
        }
        else if constexpr (std::ranges::range<Type>)
        {
            return FindRangeSize(aObject);
        }
        else if constexpr (std::derived_from<Type, IStreamable>)
        {
            return FindStreamableSize(aObject);
        }
        else if constexpr (is_any_pointer<Type>)
        {
            return FindObjectSize(*aObject);
        }
        else if constexpr (is_standard_layout_no_pointer<Type>)
        {
            return sizeof(Type);
        }
        else
        {
            static_assert(always_false<Type>, "Type is not accepted!");
        }
    }

    template <typename Type>
    [[nodiscard]] static constexpr Size::size_max FindStreamableSize(Type &aStreamable) noexcept
    {
        static_assert(std::derived_from<Type, IStreamable>, "Type is not a streamable!");

        Size::size_max size{};
        if constexpr (is_any_pointer<Type>)
        {
            size = aStreamable->FindParseSize();
        }
        else
        {
            size = aStreamable.FindParseSize();
        }

        return Size::FindRequiredBytes(size) + size;
    }

    template <typename Type> [[nodiscard]] static constexpr Size::size_max FindRangeSize(Type &aRange) noexcept
    {
        Size::size_max size = Size::FindRequiredBytes(GetRangeCount(aRange));
        if constexpr (FindRangeRank<Type>() > 1)
        {
            for (auto &object : aRange)
            {
                size += FindRangeSize(object);
            }
        }
        else
        {
            size += FindRangeRank1Size(aRange);
        }

        return size;
    }

    template <typename Type> [[nodiscard]] static constexpr Size::size_max FindRangeRank1Size(Type &aRange) noexcept
    {
        static_assert(std::ranges::range<Type>, "Type is not a range!");

        using TypeValueType = typename Type::value_type;

        Size::size_max size{};
        if constexpr (is_range_standard_layout<Type>)
        {
            if constexpr (is_wstring<Type>)
            {
                size += Converter::FindUTF8Size(aRange);
            }
            else
            {
                size += GetRangeCount(aRange) * sizeof(TypeValueType);
            }
        }
        else
        {
            for (auto &object : aRange)
            {
                size += FindObjectSize(object);
            }
        }

        return size;
    }
};

class StreamReader
{
  public:
    constexpr StreamReader(Stream &aStream) noexcept : mStream(&aStream)
    {
    }

    constexpr StreamReader(StreamReader &&aStreamReader) noexcept
    {
        *this = std::move(aStreamReader);
    }

    template <typename Type, typename... Types> constexpr void ReadAll(Type &aObject, Types &...aObjects)
    {
        using TypeRaw = std::remove_cvref_t<Type>;

        Read<TypeRaw>(aObject);

        if constexpr (sizeof...(aObjects))
        {
            ReadAll<Types...>(aObjects...);
        }
    }

    constexpr void ReadAll()
    {
    }

    template <typename FunctionSeek>
    inline decltype(auto) Peek(const FunctionSeek &aFunctionSeek, const Size::size_max aOffset = 0)
    {
        mStream->Peek(aFunctionSeek, aOffset);
        return *this;
    }

    constexpr StreamReader &operator=(StreamReader &&aStreamReader) noexcept
    {
        mStream = aStreamReader.mStream;

        return *this;
    }

  private:
    Stream *mStream{};

    template <typename Type> constexpr decltype(auto) Read(Type &aObject)
    {
        if constexpr (is_optional_v<Type>)
        {
            return ReadOptional(aObject);
        }
        else if constexpr (is_variant_v<Type>)
        {
            return ReadVariant(aObject);
        }
        else if constexpr (is_tuple_v<Type>)
        {
            std::apply([&](auto &&...aArgs) { ReadAll(aArgs...); }, aObject);
            return *this;
        }
        else if constexpr (is_pair_v<Type>)
        {
            // we remove the constness of map's pair's key so we can use the already implemented Read branches
            auto &first = const_cast<std::remove_const_t<typename Type::first_type> &>(aObject.first);
            return ReadAll(first, aObject.second);
        }
        else if constexpr (std::ranges::range<Type>)
        {
            aObject = std::move(ReadRange<Type>());
            return *this;
        }
        else if constexpr (std::derived_from<Type, IStreamable>)
        {
            return ReadStreamable(aObject);
        }
        else if constexpr (is_any_pointer<Type>)
        {
            return ReadPointer(aObject);
        }
        else if constexpr (is_standard_layout_no_pointer<Type>)
        {
            return ReadObjectOfKnownSize(aObject);
        }
        else
        {
            static_assert(always_false<Type>, "Type is not accepted!");
        }
    }

    template <typename Type> constexpr decltype(auto) ReadOptional(Type &aOpt)
    {
        static_assert(is_optional_v<Type>, "Type is not an optional!");

        using TypeValueType = typename Type::value_type;

        if (ReadCount())
        {
            TypeValueType obj{};
            Read(obj);
            aOpt = std::move(obj);
        }

        return *this;
    }

    template <typename Type> constexpr decltype(auto) ReadVariant(Type &aVariant)
    {
        static_assert(is_variant_v<Type>, "Type is not a variant!");

        aVariant = variant_from_index<Type>(ReadCount());
        std::visit([&](auto &&aArg) { Read(aArg); }, aVariant);

        return *this;
    }

    template <typename Type> constexpr decltype(auto) ReadPointer(Type &aPointer)
    {
        static_assert(is_any_pointer<Type>, "Type is not a smart/raw pointer!");

        if constexpr (is_unique_ptr_v<Type>)
        {
            aPointer = std::make_unique<typename Type::element_type>();
        }
        else if constexpr (is_shared_ptr_v<Type>)
        {
            aPointer = std::make_shared<typename Type::element_type>();
        }
        else // raw pointers
        {
            aPointer = new std::remove_pointer_t<Type>;
        }

        // we treat pointers to streamable in a special way
        if constexpr (is_derived_from_pointer<Type, IStreamable>)
        {
            return ReadStreamablePtr(aPointer);
        }
        else
        {
            return Read(*aPointer);
        }
    }

    template <typename Type> constexpr decltype(auto) ReadStreamable(Type &aStreamable)
    {
        static_assert(std::derived_from<Type, IStreamable>, "Type is not a streamable!");

        aStreamable.Deserialize(mStream->Read(ReadCount()), false); // read streamable size in bytes
        return *this;
    }

    template <typename Type> constexpr decltype(auto) ReadStreamablePtr(Type &aStreamablePtr)
    {
        static_assert(is_derived_from_pointer<Type, IStreamable>, "Type is not a streamable smart/raw pointer!");

        using TypeNoPtr = std::conditional_t<std::is_pointer_v<Type>, std::remove_pointer_t<Type>,
                                             typename std::pointer_traits<Type>::element_type>;

        // we cannot use the has_method_find_derived_streamable concept because we must use the context of the
        // StreamReader that is a friend of streamables
        static_assert(
            requires(StreamReader &aStreamReader) {
                {
                    TypeNoPtr::FindDerivedStreamable(aStreamReader)
                } -> std::convertible_to<IStreamable *>;
            }, "Type doesn't have a public/protected method 'static "
               "IStreamable* FindDerivedStreamable(StreamReader &)' !");

        Peek([&](auto) {
            Stream stream(mStream->Read(ReadCount())); // read streamable size in bytes
            StreamReader streamReader(stream);

            // TODO: we let the user read n objects after wich we read again... fix it
            if constexpr (is_smart_pointer<Type>)
            {
                aStreamablePtr.reset(static_cast<TypeNoPtr *>(TypeNoPtr::FindDerivedStreamable(streamReader)));
            }
            else
            {
                aStreamablePtr = static_cast<TypeNoPtr *>(TypeNoPtr::FindDerivedStreamable(streamReader));
            }
        });

        aStreamablePtr->Deserialize(mStream->Read(ReadCount()), false);
        return *this;
    }

    template <typename Type> [[nodiscard]] constexpr Type ReadRange()
    {
        static_assert(std::ranges::range<Type>, "Type is not a range!");

        using TypeValueType = typename Type::value_type;

        Type range{};
        const auto count = ReadCount();
        RangeReserve(range, count);

        if constexpr (SizeFinder::FindRangeRank<Type>() > 1)
        {
            for (size_t i = 0; i < count; i++)
            {
                range.insert(std::ranges::cend(range), ReadRange<TypeValueType>());
            }
        }
        else
        {
            ReadRangeRank1(range, count);
        }

        return range;
    }

    template <typename Type> constexpr decltype(auto) ReadPath(Type &aRange, const Size::size_max aCount)
    {
        static_assert(is_path<Type>, "Type is not a path!");

        using TypeStringType = typename Type::string_type;

        TypeStringType pathNative{};
        ReadRangeStandardLayout(pathNative, aCount);
        aRange.assign(pathNative);

        return *this;
    }

    template <typename Type> constexpr StreamReader &ReadRangeStandardLayout(Type &aRange, const Size::size_max aCount)
    {
        static_assert(is_range_standard_layout<Type>, "Type is not a standard layout range!");

        using TypeValueType = typename Type::value_type;

        if constexpr (is_wstring<Type>)
        {
            aRange.assign(Converter::FromUTF8(mStream->Read(aCount)));
        }
        else if constexpr (is_path<Type>)
        {
            ReadPath(aRange, aCount);
        }
        else
        {
            const auto rangeView = mStream->Read(aCount * sizeof(TypeValueType));
            const auto rangePtr = reinterpret_cast<const TypeValueType *>(rangeView.data());
            aRange.assign(rangePtr, rangePtr + rangeView.size() / sizeof(TypeValueType));
        }

        return *this;
    }

    template <typename Type> constexpr decltype(auto) ReadRangeRank1(Type &aRange, const Size::size_max aCount)
    {
        static_assert(std::ranges::range<Type>, "Type is not a range!");

        using TypeValueType = typename Type::value_type;

        if constexpr (is_range_standard_layout<Type>)
        {
            ReadRangeStandardLayout(aRange, aCount);
        }
        else
        {
            for (size_t i = 0; i < aCount; i++)
            {
                TypeValueType object{};
                Read(object);
                aRange.insert(std::ranges::cend(aRange), std::move(object));
            }
        }

        return *this;
    }

    template <typename Type> constexpr decltype(auto) RangeReserve(Type &aRange, const Size::size_max aCount)
    {
        static_assert(std::ranges::range<Type>, "Type is not a range!");

        using TypeValueType = typename Type::value_type;

        Size::size_max size{};

        if constexpr (has_method_reserve<Type>)
        {
            if constexpr (std::derived_from<IStreamable, Type>)
            {
                Peek([&](auto) {
                    for (size_t i = 0; i < aCount; i++)
                    {
                        const auto sizeCurrent = ReadCount();
                        size += sizeCurrent;
                        [[maybe_unused]] auto _ = mStream->Read(sizeCurrent);
                    }
                });
            }
            else if constexpr (is_standard_layout_no_pointer<TypeValueType>)
            {
                if constexpr (is_wstring<Type>)
                {
                    Peek([&](auto) { size += Converter::FindUTF16Size(mStream->Read(aCount)); });
                }
                else
                {
                    size = aCount * sizeof(TypeValueType);
                }
            }
            else
            {
                size = aCount;
            }

            aRange.reserve(size);
        }

        return *this;
    }

    template <typename Type> constexpr decltype(auto) ReadObjectOfKnownSize(Type &aObject)
    {
        static_assert(is_standard_layout_no_pointer<Type>, "Type is not an object of known size or it is a pointer!");

        const auto view = mStream->Read(sizeof(Type));
        aObject = *reinterpret_cast<const Type *>(view.data());

        return *this;
    }

    inline Size::size_max ReadCount() noexcept
    {
        const auto size = Size::FindRequiredBytes(mStream->Current());
        return Size::MakeSize(mStream->Read(size));
    }
};

class StreamWriter
{
  public:
    constexpr StreamWriter(Stream &aStream) noexcept : mStream(&aStream)
    {
    }

    constexpr StreamWriter(StreamWriter &&aStreamWriter) noexcept
    {
        *this = std::move(aStreamWriter);
    }

    template <typename Type, typename... Types> constexpr void WriteAll(Type &aObject, Types &...aObjects)
    {
        using TypeRaw = std::remove_cvref_t<Type>;

        Write<TypeRaw>(aObject);

        if constexpr (sizeof...(aObjects))
        {
            WriteAll<Types...>(aObjects...);
        }
    }

    constexpr void WriteAll()
    {
    }

    constexpr StreamWriter &operator=(StreamWriter &&aStreamWriter) noexcept
    {
        mStream = aStreamWriter.mStream;

        return *this;
    }

  private:
    Stream *mStream{};

    template <typename Type> constexpr decltype(auto) WriteObjectOfKnownSize(const Type &aObject)
    {
        static_assert(is_standard_layout_no_pointer<Type>, "Type is not an object of known size or it is a pointer!");

        const auto objectPtr = reinterpret_cast<const uint8_t *>(&aObject);
        mStream->Write({objectPtr, sizeof(aObject)});

        return *this;
    }

    inline decltype(auto) WriteCount(const Size::size_max aSize)
    {
        mStream->Write(Size::MakeSize(aSize));
        return *this;
    }

    template <typename Type> constexpr decltype(auto) WriteStreamable(Type &aStreamable)
    {
        static_assert(std::derived_from<Type, IStreamable>, "Type is not a streamable!");

        auto stream(std::move(aStreamable.Serialize()));
        const auto streamView = stream.View();

        // we write the size in bytes of the stream
        WriteCount(streamView.size());
        return mStream->Write(streamView);
    }

    template <typename Type> constexpr decltype(auto) WriteRange(Type &aRange)
    {
        static_assert(std::ranges::range<Type>, "Type is not a range!");

        if constexpr (SizeFinder::FindRangeRank<Type>() > 1)
        {
            WriteCount(SizeFinder::GetRangeCount(aRange));
            for (auto &object : aRange)
            {
                WriteRange(object);
            }
        }
        else
        {
            WriteRangeRank1(aRange);
        }

        return *this;
    }

    template <typename Type> constexpr decltype(auto) WriteRangeStandardLayout(const Type &aRange)
    {
        static_assert(is_range_standard_layout<Type>, "Type is not a standard layout range!");

        using TypeValueType = typename Type::value_type;

        if constexpr (is_wstring<Type>)
        {
            WriteRangeStandardLayout(Converter::ToUTF8(aRange));
        }
        else if constexpr (is_path<Type>)
        {
            WriteRangeStandardLayout(aRange.native());
        }
        else
        {
            WriteCount(SizeFinder::GetRangeCount(aRange));

            const auto rangePtr = reinterpret_cast<const uint8_t *>(std::ranges::data(aRange));
            const auto rangeSize = SizeFinder::GetRangeCount(aRange) * sizeof(TypeValueType);
            mStream->Write({rangePtr, rangeSize});
        }

        return *this;
    }

    template <typename Type> constexpr decltype(auto) WriteRangeRank1(Type &aRange)
    {
        static_assert(SizeFinder::FindRangeRank<Type>() == 1, "Type is not a rank 1 range!");

        if constexpr (is_range_standard_layout<Type>)
        {
            WriteRangeStandardLayout(aRange);
        }
        else
        {
            WriteCount(SizeFinder::GetRangeCount(aRange));
            for (auto &object : aRange)
            {
                Write(object);
            }
        }

        return *this;
    }

    template <typename Type> constexpr decltype(auto) WriteVariant(Type &aVariant)
    {
        static_assert(is_variant_v<Type>, "Type is not a variant!");

        WriteCount(aVariant.index());
        std::visit([&](auto &&aArg) { Write(aArg); }, aVariant);

        return *this;
    }

    template <typename Type> constexpr decltype(auto) WriteOptional(Type &aOpt)
    {
        static_assert(is_optional_v<Type>, "Type is not an optional!");

        WriteCount(aOpt.has_value());
        if (aOpt.has_value())
        {
            Write(*aOpt);
        }

        return *this;
    }

    template <typename Type> constexpr decltype(auto) Write(Type &aObject)
    {
        if constexpr (is_optional_v<Type>)
        {
            return WriteOptional(aObject);
        }
        else if constexpr (is_variant_v<Type>)
        {
            return WriteVariant(aObject);
        }
        else if constexpr (is_tuple_v<Type>)
        {
            std::apply([&](auto &&...aArgs) { WriteAll(aArgs...); }, aObject);
            return *this;
        }
        else if constexpr (is_pair_v<Type>)
        {
            // we remove the constness of map's pair's key so we can use the already implemented Write branches
            auto &first = const_cast<std::remove_const_t<typename Type::first_type> &>(aObject.first);
            return WriteAll(first, aObject.second);
        }
        else if constexpr (std::ranges::range<Type>)
        {
            return WriteRange(aObject);
        }
        else if constexpr (std::derived_from<Type, IStreamable>)
        {
            return WriteStreamable(aObject);
        }
        else if constexpr (is_any_pointer<Type>)
        {
            return Write(*aObject);
        }
        else if constexpr (is_standard_layout_no_pointer<Type>)
        {
            return WriteObjectOfKnownSize(aObject);
        }
        else
        {
            static_assert(always_false<Type>, "Type is not accepted!");
        }
    }
};

class IStreamable
{
    friend class SizeFinder;
    friend class StreamWriter;
    friend class StreamReader;

    Stream mStream;

  public:
    [[nodiscard]] inline decltype(auto) Serialize()
    {
        ToStream();
        return Release();
    }

    inline void Deserialize(Stream &&aStream, const bool aClear = true)
    {
        Swap(std::move(aStream));
        FromStream();

        if (aClear)
        {
            mStream.Clear();
        }
    }

  protected:
    StreamWriter mStreamWriter;
    StreamReader mStreamReader;

    constexpr IStreamable() noexcept : mStreamWriter(mStream), mStreamReader(mStream)
    {
    }

    virtual void ToStream() = 0;
    virtual void FromStream() = 0;

    [[nodiscard]] virtual Size::size_max FindParseSize() = 0;

    [[nodiscard]] constexpr Stream &&Release() noexcept
    {
        return std::move(mStream);
    }

    constexpr Stream &Swap(Stream &&aStream) noexcept
    {
        mStream = std::move(aStream);
        mStreamWriter = StreamWriter(mStream);
        mStreamReader = StreamReader(mStream);

        return mStream;
    }

    constexpr decltype(auto) Reserve(const Size::size_max aSize)
    {
        return mStream.Reserve(aSize);
    }

    constexpr IStreamable(const IStreamable &aIStreamable) noexcept : IStreamable()
    {
        *this = aIStreamable;
    }

    constexpr IStreamable(IStreamable &&aIStreamable) noexcept : IStreamable()
    {
        *this = std::move(aIStreamable);
    }

    constexpr IStreamable &operator=(const IStreamable &) noexcept
    {
        return *this;
    }

    constexpr IStreamable &operator=(IStreamable &&aIStreamable) noexcept
    {
        Swap(std::move(aIStreamable.mStream));
        return *this;
    }
};
} // namespace hbann
