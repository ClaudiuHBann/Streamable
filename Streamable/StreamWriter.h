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

    constexpr decltype(auto) WriteCount(const uint64_t aSize)
    {
        mStream->Write(Size::MakeSize(static_cast<Size::size_max>(aSize)));
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

        return *this;
    }

    template <typename Type> constexpr decltype(auto) WriteRangeRank1(Type &aRange)
    {
        static_assert(std::ranges::range<Type>, "Type is not a range!");

        using TypeValueType = typename Type::value_type;

        if constexpr (is_range_std_lay<Type>)
        {
            const char *rangePtr{};
            if constexpr (is_path<Type>)
            {
                rangePtr = reinterpret_cast<const char *>(aRange.native().data());
            }
            else
            {
                rangePtr = reinterpret_cast<const char *>(std::ranges::data(aRange));
            }

            const auto rangeSize = SizeFinder::GetRangeCount(aRange) * sizeof(TypeValueType);
            mStream->Write({rangePtr, static_cast<size_t>(rangeSize)});
        }
        else
        {
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
} // namespace hbann
