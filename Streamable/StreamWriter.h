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
        static_assert(is_std_lay_no_ptr<Type>, "Type is not an object of known size or it is a pointer!");

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
        static_assert(is_range_std_lay<Type>, "Type is not a standard layout range!");

        if constexpr (std::is_same_v<Type, std::wstring>)
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
            const auto rangeSize = SizeFinder::GetRangeCount(aRange) * sizeof(typename Type::value_type);
            mStream->Write({rangePtr, rangeSize});
        }

        return *this;
    }

    template <typename Type> constexpr decltype(auto) WriteRangeRank1(Type &aRange)
    {
        static_assert(SizeFinder::FindRangeRank<Type>() == 1, "Type is not a rank 1 range!");

        if constexpr (is_range_std_lay<Type>)
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
            // TODO: can we remove this workaround for std::map's key ?
            // we remove the constness of the std::pair::first_type because of the std::map::value_type::first_type
            auto &first = const_cast<std::remove_const_t<typename Type::first_type> &>(aObject.first);
            return WriteAll(first, aObject.second);
        }
        else if constexpr (std::ranges::range<Type>)
        {
            return WriteRange(aObject);
        }
        else if constexpr (is_base_of_no_ptr<IStreamable, Type>)
        {
            return WriteStreamable(aObject);
        }
        else if constexpr (is_std_lay_no_ptr<Type>)
        {
            return WriteObjectOfKnownSize(aObject);
        }
        else
        {
            static_assert(always_false<Type>, "Type is not accepted!");
        }
    }
};
} // namespace hbann
