/*
    Copyright (c) 2023 Claudiu HBann
    
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
#include <span>
#include <string>
#include <variant>
#include <vector>

// Streamable
constexpr auto STREAMABLE_INTERFACE_NAME = "IStreamable";

#define STREAMABLE_DEFINE_FROM_STREAM(baseClass, ...)                                                                  \
  public:                                                                                                              \
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
  public:                                                                                                              \
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
    friend class ::hbann::StreamReader;                                                                                \
    friend class ::hbann::StreamWriter;

#define STATIC_ASSERT_HAS_ISTREAMABLE_BASE(baseClass)                                                                  \
    static_assert(std::is_base_of_v<::hbann::IStreamable, baseClass>, "The class must inherit a streamable!");

#define STREAMABLE_DEFINE(baseClass, ...)                                                                              \
    STATIC_ASSERT_HAS_ISTREAMABLE_BASE(baseClass)                                                                      \
    STREAMABLE_DEFINE_INTRUSIVE                                                                                        \
    STREAMABLE_DEFINE_TO_STREAM(baseClass, __VA_ARGS__)                                                                \
    STREAMABLE_DEFINE_FROM_STREAM(baseClass, __VA_ARGS__)                                                              \
    STREAMABLE_DEFINE_FIND_PARSE_SIZE(baseClass, __VA_ARGS__)

namespace hbann
{
template <typename> constexpr auto always_false = false;

template <typename Container>
concept has_method_reserve =
    std::ranges::contiguous_range<Container> && requires(Container &aContainer) { aContainer.reserve(size_t(0)); };

template <typename Type>
concept is_pointer_unique = std::is_same_v<std::remove_cvref_t<Type>, std::unique_ptr<typename Type::value_type>>;

template <typename Type>
concept is_pointer_shared = std::is_same_v<std::remove_cvref_t<Type>, std::shared_ptr<typename Type::value_type>>;

template <typename Type>
concept is_pointer = std::is_pointer_v<Type> || is_pointer_unique<Type> || is_pointer_shared<Type>;

template <typename Type>
concept is_std_lay_no_ptr = std::is_standard_layout_v<Type> && !is_pointer<Type>;

template <typename Base, typename Derived>
concept is_base_of_no_ptr = std::is_base_of_v<Base, std::remove_pointer_t<Derived>>;

template <typename Type>
concept is_path = std::is_same_v<std::remove_cvref_t<Type>, std::filesystem::path>;

template <typename Container>
concept is_range_std_lay =
    (std::ranges::contiguous_range<Container> && is_std_lay_no_ptr<typename Container::value_type>) ||
    is_path<Container>;

template <typename Container>
concept has_method_size = requires(Container &aContainer) { std::ranges::size(aContainer); };

template <typename Class>
concept has_method_find_derived_streamable = requires(StreamReader &aStreamReader) {
    {
        Class::FindDerivedStreamable(aStreamReader)
    } -> std::convertible_to<IStreamable *>;
};

constexpr bool static_equal(const char *aString1, const char *aString2) noexcept
{
    return *aString1 == *aString2 && (!*aString1 || static_equal(aString1 + 1, aString2 + 1));
}

/*
    Format: first 3 bits + the actual size at last

    The first 3 bits represent how many bytes are necessary to represent the actual size with those 3 bits and they are
   written at the most left side of the 1-8 bytes.

    The actual size is written to the left most side of the 1-8 bytes.
*/
class Size
{
  public:
    using size_max = size_t; // size_t / 8
    using span = std::span<const char>;

    [[nodiscard]] static constexpr auto FindRequiredBytes(const char aSize) noexcept
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
        static char SIZE_AS_CHARS[SIZE_MAX_IN_BYTES]{};
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
        static char SIZE_AS_CHARS[SIZE_MAX_IN_BYTES]{};
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

class Stream
{
    friend class StreamReader;

  public:
    using vector = std::vector<char>;
    using span = std::span<const char>;
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

    template <typename FunctionSeek> inline decltype(auto) Seek(const FunctionSeek &aFunctionSeek)
    {
        const auto readIndex = mReadIndex;
        aFunctionSeek(readIndex);
        mReadIndex = readIndex;

        return *this;
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
        if constexpr (std::ranges::range<Type>)
        {
            return FindRangeSize(aObject);
        }
        else if constexpr (is_base_of_no_ptr<IStreamable, Type>)
        {
            Size::size_max size{};
            if constexpr (is_pointer<Type>)
            {
                size = static_cast<IStreamable *>(aObject)->FindParseSize();
            }
            else
            {
                size = static_cast<IStreamable *>(&aObject)->FindParseSize();
            }

            return Size::FindRequiredBytes(size);
        }
        else if constexpr (is_std_lay_no_ptr<Type>)
        {
            return sizeof(Type);
        }
        else
        {
            static_assert(always_false<Type>, "Type is not accepted!");
        }
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
        if constexpr (is_range_std_lay<Type>)
        {
            if constexpr (std::is_same_v<Type, std::wstring>)
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

    static constexpr Size::size_max FindParseSize() noexcept
    {
        return 0;
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

    constexpr StreamWriter &operator=(StreamWriter &&aStreamWriter) noexcept
    {
        mStream = aStreamWriter.mStream;

        return *this;
    }

  private:
    Stream *mStream{};

    template <typename Type> constexpr decltype(auto) WriteObjectOfKnownSize(const Type &aObject)
    {
        static_assert(is_std_lay_no_ptr<Type>, "Type is not an object of known size or it is a pointer!");

        const auto objectPtr = reinterpret_cast<const char *>(&aObject);
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
        static_assert(is_base_of_no_ptr<IStreamable, Type>, "Type is not a streamable (pointer)!");

        Stream stream{};
        if constexpr (is_pointer<Type>)
        {
            stream = std::move(aStreamable->Serialize());
        }
        else
        {
            stream = std::move(aStreamable.Serialize());
        }
        const auto streamView = stream.View();

        // we write the size in bytes of the stream
        WriteCount(streamView.size());
        return mStream->Write(streamView);
    }

    template <typename Type> constexpr decltype(auto) WriteRange(Type &aRange)
    {
        static_assert(std::ranges::range<Type>, "Type is not a range!");

        using TypeValueType = typename Type::value_type;

        if constexpr (SizeFinder::FindRangeRank<Type>() > 1)
        {
            WriteCount(SizeFinder::GetRangeCount(aRange));
            for (auto &object : aRange)
            {
                WriteRange<TypeValueType>(object);
            }
        }
        else
        {
            WriteRangeRank1<Type>(aRange);
        }

        return *this;
    }

    template <typename Type> constexpr decltype(auto) WriteRangeRank1(Type &aRange)
    {
        static_assert(std::ranges::range<Type>, "Type is not a range!");

        using TypeValueType = typename Type::value_type;

        if constexpr (is_range_std_lay<Type>)
        {
            if constexpr (std::is_same_v<Type, std::wstring>)
            {
                const auto stringUTF8 = Converter::ToUTF8(aRange);
                WriteCount(SizeFinder::GetRangeCount(stringUTF8));
                mStream->Write(stringUTF8);
            }
            else if constexpr (is_path<Type>)
            {
                WriteCount(SizeFinder::GetRangeCount(aRange));

                const auto rangePtr = reinterpret_cast<const char *>(aRange.native().data());
                const auto rangeSize = SizeFinder::GetRangeCount(aRange) * sizeof(TypeValueType);
                mStream->Write({rangePtr, rangeSize});
            }
            else
            {
                WriteCount(SizeFinder::GetRangeCount(aRange));

                const auto rangePtr = reinterpret_cast<const char *>(std::ranges::data(aRange));
                const auto rangeSize = SizeFinder::GetRangeCount(aRange) * sizeof(TypeValueType);
                mStream->Write({rangePtr, rangeSize});
            }
        }
        else
        {
            WriteCount(SizeFinder::GetRangeCount(aRange));
            for (auto &object : aRange)
            {
                Write<TypeValueType>(object);
            }
        }

        return *this;
    }

    template <typename Type> constexpr decltype(auto) Write(Type &aObject)
    {
        if constexpr (std::ranges::range<Type>)
        {
            return WriteRange<Type>(aObject);
        }
        else if constexpr (is_base_of_no_ptr<IStreamable, Type>)
        {
            return WriteStreamable<Type>(aObject);
        }
        else if constexpr (is_std_lay_no_ptr<Type>)
        {
            return WriteObjectOfKnownSize<Type>(aObject);
        }
        else
        {
            static_assert(always_false<Type>, "Type is not accepted!");
        }
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

    constexpr StreamReader &operator=(StreamReader &&aStreamReader) noexcept
    {
        mStream = aStreamReader.mStream;

        return *this;
    }

  private:
    Stream *mStream{};

    template <typename Type> constexpr decltype(auto) Read(Type &aObject)
    {
        if constexpr (std::ranges::range<Type>)
        {
            aObject = std::move(ReadRange<Type>());
            return *this;
        }
        else if constexpr (is_base_of_no_ptr<IStreamable, Type>)
        {
            return ReadStreamableX<Type>(aObject);
        }
        else if constexpr (is_std_lay_no_ptr<Type>)
        {
            return ReadObjectOfKnownSize<Type>(aObject);
        }
        else
        {
            static_assert(always_false<Type>, "Type is not accepted!");
        }
    }

    template <typename Type> constexpr decltype(auto) ReadStreamableX(Type &aStreamable)
    {
        if constexpr (is_pointer<Type>)
        {
            return ReadStreamablePtr<Type>(aStreamable);
        }
        else
        {
            return ReadStreamable<Type>(aStreamable);
        }
    }

    template <typename Type> constexpr decltype(auto) ReadStreamable(Type &aStreamable)
    {
        static_assert(std::is_base_of_v<IStreamable, Type>, "Type is not a streamable!");

        aStreamable.Deserialize(mStream->Read(ReadCount()), false); // read streamable size in bytes
        return *this;
    }

    template <typename Type> constexpr decltype(auto) ReadStreamablePtr(Type &aStreamablePtr)
    {
        using TypeNoPtr = std::remove_pointer_t<Type>;

        static_assert(std::is_base_of_v<IStreamable, TypeNoPtr>, "Type is not a streamable pointer!");
        static_assert(has_method_find_derived_streamable<TypeNoPtr>,
                      "Type doesn't have method 'static IStreamable* FindDerivedStreamable(StreamReader &)' !");

        mStream->Seek([&](auto) {
            Stream stream(mStream->Read(ReadCount())); // read streamable size in bytes
            StreamReader streamReader(stream);
            // TODO: we let the user read n objects after wich we read again... fix it
            aStreamablePtr = static_cast<Type>(TypeNoPtr::FindDerivedStreamable(streamReader));
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
        RangeReserve<Type>(range, count);

        if constexpr (SizeFinder::FindRangeRank<Type>() > 1)
        {
            for (size_t i = 0; i < count; i++)
            {
                range.insert(std::ranges::cend(range), ReadRange<TypeValueType>());
            }
        }
        else
        {
            ReadRangeRank1<Type>(range, count);
        }

        return range;
    }

    template <typename Type> constexpr decltype(auto) ReadRangeRank1(Type &aRange, const Size::size_max aCount)
    {
        static_assert(std::ranges::range<Type>, "Type is not a range!");

        using TypeValueType = typename Type::value_type;

        if constexpr (std::is_same_v<Type, std::wstring>)
        {
            aRange.assign(Converter::FromUTF8(mStream->Read(aCount)));
        }
        else if constexpr (is_range_std_lay<Type>)
        {
            const auto rangeView = mStream->Read(aCount * sizeof(TypeValueType));
            const auto rangePtr = reinterpret_cast<const TypeValueType *>(rangeView.data());
            aRange.assign(rangePtr, rangePtr + rangeView.size() / sizeof(TypeValueType));
        }
        else
        {
            for (size_t i = 0; i < aCount; i++)
            {
                TypeValueType object{};
                Read<TypeValueType>(object);
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

        // TODO: handle range multiple ranks
        if constexpr (has_method_reserve<Type>)
        {
            if constexpr (is_base_of_no_ptr<IStreamable, Type>)
            {
                mStream->Seek([&](auto) {
                    for (size_t i = 0; i < aCount; i++)
                    {
                        const auto sizeCurrent = ReadCount();
                        size += sizeCurrent;
                        auto seek = mStream->Read(sizeCurrent);
                        seek;
                    }
                });
            }
            else if constexpr (is_std_lay_no_ptr<TypeValueType>)
            {
                size = aCount * sizeof(TypeValueType);
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
        static_assert(is_std_lay_no_ptr<Type>, "Type is not an object of known size or it is a pointer!");

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
