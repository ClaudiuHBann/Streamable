#pragma once

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
        using TypeRaw = get_raw_t<Type>;

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

    template <typename Type> inline void WriteObjectOfKnownSize(const Type &aObject)
    {
        static_assert(is_std_lay_no_ptr<Type>, "Type is not an object of known size or it is a pointer!");

        const auto objectPtr = reinterpret_cast<const char *>(&aObject);
        mStream->Write(objectPtr, sizeof(aObject));
    }

    inline void WriteCount(const size_t aSize)
    {
        WriteObjectOfKnownSize<size_range>((size_range)aSize);
    }

    template <typename Type> void WriteStreamable(Type &aStreamable)
    {
        static_assert(is_base_of_no_ptr<IStreamable, Type>, "Type is not a streamable (pointer)!");

        Stream stream{};
        if constexpr (std::is_pointer_v<Type>)
        {
            stream = std::move(aStreamable->Serialize());
        }
        else
        {
            stream = std::move(aStreamable.Serialize());
        }
        const auto streamView = stream.View();

        WriteCount(streamView.size()); // we write the size in bytes of the stream
        mStream->Write(streamView.data(), streamView.size());
    }

    template <typename Type> constexpr void WriteRange(Type &aRange)
    {
        static_assert(std::ranges::range<Type>, "Type is not a range!");

        using TypeValueType = typename Type::value_type;

        WriteCount(SizeFinder::GetRangeCount(aRange));

        if constexpr (SizeFinder::FindRangeRank<Type>() > 1)
        {
            for (auto &object : aRange)
            {
                WriteRange<TypeValueType>(object);
            }
        }
        else
        {
            WriteRangeRank1<Type>(aRange);
        }
    }

    template <typename Type> constexpr void WriteRangeRank1(Type &aRange)
    {
        static_assert(std::ranges::range<Type>, "Type is not a range!");

        using TypeValueType = typename Type::value_type;

        if constexpr (is_range_std_lay<Type>)
        {
            if constexpr (is_path<Type>)
            {
                const auto rangePtr = reinterpret_cast<const char *>(aRange.native().data());
                const auto rangeSize = SizeFinder::GetRangeCount(aRange) * sizeof(TypeValueType);
                mStream->Write(rangePtr, rangeSize);
            }
            else
            {
                const auto rangePtr = reinterpret_cast<const char *>(std::ranges::data(aRange));
                const auto rangeSize = SizeFinder::GetRangeCount(aRange) * sizeof(TypeValueType);
                mStream->Write(rangePtr, rangeSize);
            }
        }
        else
        {
            for (auto &object : aRange)
            {
                Write<TypeValueType>(object);
            }
        }
    }

    template <typename Type> constexpr void Write(Type &aObject)
    {
        if constexpr (is_std_lay_no_ptr<Type>)
        {
            WriteObjectOfKnownSize<Type>(aObject);
        }
        else if constexpr (is_base_of_no_ptr<IStreamable, Type>)
        {
            WriteStreamable<Type>(aObject);
        }
        else if constexpr (std::ranges::range<Type>)
        {
            WriteRange<Type>(aObject);
        }
        else
        {
            static_assert(always_false<Type>, "Type is not accepted!");
        }
    }
};
} // namespace hbann
