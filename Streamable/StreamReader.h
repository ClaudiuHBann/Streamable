/*
    Copyright (c) 2023 Claudiu HBann

    See LICENSE for the full terms of the MIT License.
*/

#pragma once

#include "Converter.h"
#include "SizeFinder.h"
#include "Stream.h"

namespace hbann
{
class IStreamable;

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
        if constexpr (is_tuple_v<Type> || is_pair_v<Type>)
        {
            std::apply([this](auto &&...aArgs) { ReadAll(aArgs...); }, aObject);
        }
        else if constexpr (std::ranges::range<Type>)
        {
            aObject = std::move(ReadRange<Type>());
            return *this;
        }
        else if constexpr (is_base_of_no_ptr<IStreamable, Type>)
        {
            return ReadStreamableX(aObject);
        }
        else if constexpr (is_std_lay_no_ptr<Type>)
        {
            return ReadObjectOfKnownSize(aObject);
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
            return ReadStreamablePtr(aStreamable);
        }
        else
        {
            return ReadStreamable(aStreamable);
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

        Peek([&](auto) {
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

        Type range{};
        const auto count = ReadCount();
        RangeReserve(range, count);

        if constexpr (SizeFinder::FindRangeRank<Type>() > 1)
        {
            for (size_t i = 0; i < count; i++)
            {
                range.insert(std::ranges::cend(range), ReadRange<typename Type::value_type>());
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

        if constexpr (std::is_same_v<std::wstring, typename Type::string_type>)
        {
            aRange.assign(Converter::FromUTF8(mStream->Read(aCount)));
        }
        else
        {
            ReadRangeStandardLayout(aRange, aCount);
        }

        return *this;
    }

    template <typename Type> constexpr decltype(auto) ReadRangeStandardLayout(Type &aRange, const Size::size_max aCount)
    {
        static_assert(is_range_std_lay<Type>, "Type is not a standard layout range!");

        using TypeValueType = typename Type::value_type;

        if constexpr (std::is_same_v<Type, std::wstring>)
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

        if constexpr (is_range_std_lay<Type>)
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

        // TODO: handle range multiple ranks
        if constexpr (has_method_reserve<Type>)
        {
            if constexpr (is_base_of_no_ptr<IStreamable, Type>)
            {
                Peek([&](auto) {
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
} // namespace hbann
