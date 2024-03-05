/*
    Copyright (c) 2023 Claudiu HBann

    See LICENSE for the full terms of the MIT License.
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

#define DEFINE_TT1(name, tn, t)                                                                                        \
    namespace hbann                                                                                                    \
    {                                                                                                                  \
    namespace impl                                                                                                     \
    {                                                                                                                  \
    template <typename> struct is_##name : std::false_type                                                             \
    {                                                                                                                  \
    };                                                                                                                 \
                                                                                                                       \
    template <tn> struct is_##name##<std::##name##<t>> : std::true_type                                                \
    {                                                                                                                  \
    };                                                                                                                 \
    }                                                                                                                  \
                                                                                                                       \
    template <typename Type> constexpr bool is_##name##_v = impl::is_##name##<Type>::value;                            \
    }

#define DEFINE_TT2(name, tn1, tn2, t1, t2)                                                                             \
    namespace hbann                                                                                                    \
    {                                                                                                                  \
    namespace impl                                                                                                     \
    {                                                                                                                  \
    template <typename> struct is_##name : std::false_type                                                             \
    {                                                                                                                  \
    };                                                                                                                 \
                                                                                                                       \
    template <tn1, tn2> struct is_##name##<std::##name##<t1, t2>> : std::true_type                                     \
    {                                                                                                                  \
    };                                                                                                                 \
    }                                                                                                                  \
                                                                                                                       \
    template <typename Type> constexpr bool is_##name##_v = impl::is_##name##<Type>::value;                            \
    }

DEFINE_TT2(pair, typename TypeFirst, typename TypeSecond, TypeFirst, TypeSecond);
DEFINE_TT1(tuple, typename... Types, Types...);
DEFINE_TT1(variant, typename... Types, Types...);
DEFINE_TT1(optional, typename Type, Type);
DEFINE_TT1(unique_ptr, typename Type, Type);
DEFINE_TT1(shared_ptr, typename Type, Type);

namespace hbann
{
template <typename> constexpr auto always_false = false;

// TODO: what if the container has a method "reserve" that doesn't actually reserve memory
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
} // namespace hbann

/*
    TODO:
         - check SizeFinder for added type support and for incorrect use of the Size::findrequired bytes and etc...
         - add separated examples
         - remake tests

    FEATURES:
         - support for multiple inheritance of streamables
         - instead of reading object to jump over the value, create a jump method

    UX:
         - when finding derived class from base class pointer, add a tuple representing the types that can be read
   and make the user access the objects by index so can't read a bad object

    WARNING:
         - when adding a new feature try to implement the serialize and deserialize algorithms with SizeFinder for
   the object and, if possible, the reserve method
*/
