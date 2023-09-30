#pragma once

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
        using TypeRaw = get_raw_t<Type>;

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

    template <typename Type> [[nodiscard]] constexpr void Read(Type &aObject)
    {
        if constexpr (is_std_lay_no_ptr<Type>)
        {
            ReadObjectOfKnownSize<Type>(aObject);
        }
        else if constexpr (is_base_of_no_ptr<IStreamable, Type>)
        {
            ReadStreamableX<Type>(aObject);
        }
        else if constexpr (std::ranges::range<Type>)
        {
            aObject = std::move(ReadRange<Type>());
        }
        else
        {
            static_assert(always_false<Type>, "Type is not accepted!");
        }
    }

    template <typename Type> [[nodiscard]] constexpr void ReadStreamableX(Type &aStreamable)
    {
        if constexpr (std::is_pointer_v<Type>)
        {
            ReadStreamablePtr<Type>(aStreamable);
        }
        else
        {
            ReadStreamable<Type>(aStreamable);
        }
    }

    template <typename Type> [[nodiscard]] constexpr void ReadStreamable(Type &aStreamable)
    {
        static_assert(std::is_base_of_v<IStreamable, Type>, "Type is not a streamable!");

        DISCARD(aStreamable.FromStream(mStream->Read(ReadCount()))); // read streamable size in bytes
    }

    template <typename Type> [[nodiscard]] constexpr void ReadStreamablePtr(Type &aStreamablePtr)
    {
        using TypeNoPtr = std::remove_pointer_t<Type>;

        static_assert(std::is_base_of_v<IStreamable, TypeNoPtr>, "Type is not a streamable pointer!");

        mStream->Seek([&](auto) {
            TypeNoPtr typeNoPtr{};
            Stream stream(mStream->Read(ReadCount())); // read streamable size in bytes
            StreamReader streamReader(stream);
            // TODO: we let the user read n objects after wich we read again... fix it
            aStreamablePtr = static_cast<Type>(typeNoPtr.FindDerivedStreamable(streamReader));
        });

        DISCARD(aStreamablePtr->FromStream(mStream->Read(ReadCount())));
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

    template <typename Type> [[nodiscard]] constexpr void ReadRangeRank1(Type &aRange, const size_t aCount)
    {
        static_assert(std::ranges::range<Type>, "Type is not a range!");

        using TypeValueType = typename Type::value_type;

        if constexpr (is_range_std_lay<Type>)
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
                aRange.insert(std::ranges::cend(aRange), object);
            }
        }
    }

    template <typename Type> [[nodiscard]] constexpr void RangeReserve(Type &aRange, const size_t aCount)
    {
        static_assert(std::ranges::range<Type>, "Type is not a range!");

        using TypeValueType = typename Type::value_type;

        // TODO: handle range multiple ranks
        if constexpr (has_method_reserve<Type>)
        {
            if constexpr (is_std_lay_no_ptr<TypeValueType>)
            {
                aRange.reserve(aCount * sizeof(TypeValueType));
            }
            else if constexpr (is_base_of_no_ptr<IStreamable, Type>)
            {
                size_t sizeTotal{};

                mStream->Seek([&](auto) {
                    for (size_t i = 0; i < aCount; i++)
                    {
                        const auto sizeCurrent = ReadCount();
                        sizeTotal += sizeCurrent;
                        mStream->Read(sizeCurrent);
                    }
                });

                aRange.reserve(sizeTotal);
            }
            else
            {
                aRange.reserve(aCount);
            }
        }
    }

    template <typename Type> [[nodiscard]] constexpr void ReadObjectOfKnownSize(Type &aObject)
    {
        static_assert(is_std_lay_no_ptr<Type>, "Type is not an object of known size or it is a pointer!");

        const auto view = mStream->Read(sizeof(Type));
        aObject = *reinterpret_cast<const Type *>(view.data());
    }

    constexpr size_t ReadCount()
    {
        size_range size{};
        ReadObjectOfKnownSize<size_range>(size);
        return size;
    }
};
} // namespace hbann
