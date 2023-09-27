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

        aObject = Read<TypeRaw>();

        if constexpr (sizeof...(aObjects))
        {
            ReadAll(aObjects...);
        }
    }

    constexpr StreamReader &operator=(StreamReader &&aStreamReader) noexcept
    {
        mStream = aStreamReader.mStream;

        return *this;
    }

  private:
    Stream *mStream{};

    template <typename Type> [[nodiscard]] constexpr decltype(auto) Read()
    {
        if constexpr (std::is_standard_layout_v<Type> && !std::is_pointer_v<Type>)
        {
            return ReadObjectOfKnownSize<Type>();
        }
        else if constexpr (std::is_base_of_v<IStreamable, std::remove_pointer_t<Type>>)
        {
            if constexpr (std::is_pointer_v<Type>)
            {
                return ReadStreamablePtr<Type>();
            }
            else
            {
                return ReadStreamable<Type>();
            }
        }
        else if constexpr (std::ranges::range<Type>)
        {
            return ReadRange<Type>();
        }
        else
        {
            static_assert(always_false<Type>, "Type is not accepted!");
        }
    }

    template <typename Type> [[nodiscard]] constexpr Type ReadStreamable() noexcept
    {
        static_assert(std::is_base_of_v<IStreamable, Type>, "Type is not a streamable!");

        Type streamable{};
        streamable.FromStream(mStream->Read(ReadCount())); // read streamable size in bytes
        return streamable;
    }

    template <typename Type> [[nodiscard]] constexpr Type ReadStreamablePtr() noexcept
    {
        using TypeNoPtr = std::remove_pointer_t<Type>;

        static_assert(std::is_base_of_v<IStreamable, TypeNoPtr>, "Type is not a streamable pointer!");

        // TODO: wtf is this brada?
        const auto readIndex = mStream->GetReadIndex();

        TypeNoPtr typeNoPtr{};
        Stream stream(mStream->Read(ReadCount())); // read streamable size in bytes
        StreamReader streamReader(stream);
        auto streamablePtr(typeNoPtr.FindDerivedStreamable(streamReader));

        mStream->SetReadIndex(readIndex);

        [[maybe_unused]] auto _(streamablePtr->FromStream(mStream->Read(ReadCount()))); // read streamable size in bytes
        return static_cast<Type>(streamablePtr);
    }

    template <typename Type> [[nodiscard]] constexpr decltype(auto) ReadRange()
    {
        static_assert(std::ranges::range<Type>, "Type is not a range!");

        using TypeValueType = typename Type::value_type;

        Type range{};
        const auto count = ReadCount();
        if constexpr (has_method_reserve_v<Type>)
        {
            if constexpr (std::is_standard_layout_v<TypeValueType> && !std::is_pointer_v<TypeValueType>)
            {
                range.reserve(count * sizeof(TypeValueType));
            }
            else
            {
                range.reserve(count);
            }
        }

        if constexpr (SizeFinder::FindRangeRank<Type>() > 1)
        {
            for (size_t i = 0; i < count; i++)
            {
                range.insert(std::ranges::cend(range), ReadRange<TypeValueType>());
            }
        }
        else
        {
            if constexpr (std::ranges::contiguous_range<Type> && std::is_standard_layout_v<TypeValueType> &&
                          !std::is_pointer_v<TypeValueType>)
            {
                const auto rangeView = mStream->Read(count * sizeof(TypeValueType));
                const auto rangePtr = reinterpret_cast<const TypeValueType *>(rangeView.data());
                range.assign(rangePtr, rangeView.size() / sizeof(TypeValueType));
            }
            else
            {
                for (size_t i = 0; i < count; i++)
                {
                    range.insert(std::ranges::cend(range), Read<TypeValueType>());
                }
            }
        }

        return range;
    }

    template <typename Type> [[nodiscard]] constexpr decltype(auto) ReadObjectOfKnownSize() noexcept
    {
        static_assert(std::is_standard_layout_v<Type> && !std::is_pointer_v<Type>,
                      "Type is not an object of known size or it is a pointer!");

        return *reinterpret_cast<const Type *>(mStream->Read(sizeof(Type)).data());
    }

    constexpr size_t ReadCount() noexcept
    {
        return ReadObjectOfKnownSize<size_range>();
    }
};
} // namespace hbann
