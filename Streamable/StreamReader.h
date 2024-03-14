/*
    Copyright (c) 2024 Claudiu HBann

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

    constexpr StreamReader(const StreamReader &aStreamReader) noexcept : mStream(aStreamReader.mStream)
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

    constexpr StreamReader &operator=(const StreamReader &aStreamReader) noexcept
    {
        mStream = aStreamReader.mStream;
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
        else if constexpr (!is_derived_from_pointer<Type, IStreamable>)
        {
            // raw pointers MUST be allocated only for non streamables
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

        if constexpr (is_utf16string<Type>)
        {
            aRange.assign(Converter::Decode<Type>(mStream->Read(aCount)));
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
} // namespace hbann
