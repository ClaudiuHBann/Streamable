/*
    Copyright (c) 2024 Claudiu HBann

    See LICENSE for the full terms of the MIT License.
*/

#pragma once

#include "StreamableFWD.h"

// native
#ifdef _WIN32

#ifndef NOIME
#define NOIME
#endif // !NOIME

#ifndef NOMCX
#define NOMCX
#endif // !NOMCX

#ifndef NOMINMAX
#define NOMINMAX
#endif // !NOMINMAX

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN

#include <Windows.h>

#else
#warning "Platform does not support encoding UTF16 strings to save memory!"
#endif

// std
#include <bit>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

// Streamable
#define STREAMABLE_DEFINE_TO_STREAM_BASES                                                                              \
  private:                                                                                                             \
    template <typename Type, std::size_t tIndex = std::tuple_size_v<Type>> constexpr void ToStreamBases()              \
    {                                                                                                                  \
        static_assert(::hbann::is_tuple_v<Type>, "Type is not a tuple!");                                              \
                                                                                                                       \
        if constexpr (tIndex > 0)                                                                                      \
        {                                                                                                              \
            std::tuple_element_t<tIndex - 1, Type>::ToStream();                                                        \
            ToStreamBases<Type, tIndex - 1>();                                                                         \
        }                                                                                                              \
    }

#define STREAMABLE_DEFINE_FROM_STREAM_BASES                                                                            \
  private:                                                                                                             \
    template <typename Type, std::size_t tIndex = std::tuple_size_v<Type>> constexpr void FromStreamBases()            \
    {                                                                                                                  \
        static_assert(::hbann::is_tuple_v<Type>, "Type is not a tuple!");                                              \
                                                                                                                       \
        if constexpr (tIndex > 0)                                                                                      \
        {                                                                                                              \
            std::tuple_element_t<tIndex - 1, Type>::FromStream();                                                      \
            FromStreamBases<Type, tIndex - 1>();                                                                       \
        }                                                                                                              \
    }

#define STREAMABLE_DEFINE_BASE(...) std::tuple<__VA_ARGS__>

#define STREAMABLE_RESET_ACCESS_MODIFIER private:

#define STREAMABLE_DEFINE_FROM_STREAM(baseClasses, ...)                                                                \
  protected:                                                                                                           \
    constexpr void FromStream() override                                                                               \
    {                                                                                                                  \
        FromStreamBases<baseClasses>();                                                                                \
                                                                                                                       \
        mStreamReader.ReadAll(__VA_ARGS__);                                                                            \
    }                                                                                                                  \
                                                                                                                       \
    STREAMABLE_RESET_ACCESS_MODIFIER

#define STREAMABLE_DEFINE_TO_STREAM(baseClasses, ...)                                                                  \
  protected:                                                                                                           \
    constexpr void ToStream() override                                                                                 \
    {                                                                                                                  \
        ToStreamBases<baseClasses>();                                                                                  \
                                                                                                                       \
        mStreamWriter.WriteAll(__VA_ARGS__);                                                                           \
    }                                                                                                                  \
                                                                                                                       \
    STREAMABLE_RESET_ACCESS_MODIFIER

#define STREAMABLE_DEFINE_INTRUSIVE                                                                                    \
  private:                                                                                                             \
    friend class ::hbann::StreamReader;                                                                                \
    friend class ::hbann::StreamWriter;

#define STATIC_ASSERT_HAS_ISTREAMABLE_BASE(baseClasses)                                                                \
    static_assert(::hbann::are_derived_from_istreamable_v<baseClasses>, "The class must inherit a streamable!");

#define STATIC_ASSERT_DONT_PASS_ISTREAMABLE_AS_BASE(baseClasses)                                                       \
    static_assert(::hbann::is_not_istreamable_v<baseClasses>, "The class ::hbann::IStreamable should not be a "        \
                                                              "base!");

#define STREAMABLE_DEFINE(className, baseClasses, ...)                                                                 \
    STATIC_ASSERT_DONT_PASS_ISTREAMABLE_AS_BASE(baseClasses)                                                           \
    STATIC_ASSERT_HAS_ISTREAMABLE_BASE(baseClasses)                                                                    \
                                                                                                                       \
    STREAMABLE_DEFINE_INTRUSIVE                                                                                        \
                                                                                                                       \
    STREAMABLE_DEFINE_TO_STREAM_BASES                                                                                  \
    STREAMABLE_DEFINE_TO_STREAM(baseClasses, __VA_ARGS__)                                                              \
                                                                                                                       \
    STREAMABLE_DEFINE_FROM_STREAM_BASES                                                                                \
    STREAMABLE_DEFINE_FROM_STREAM(baseClasses, __VA_ARGS__)                                                            \
                                                                                                                       \
    STREAMABLE_RESET_ACCESS_MODIFIER

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
template <typename... Types> struct is_basic_string<std::basic_string<Types...>> : std::true_type
{
};

template <typename> struct are_derived_from_istreamable : std::false_type
{
};
template <typename... Types>
struct are_derived_from_istreamable<std::tuple<Types...>>
    : std::integral_constant<bool, (std::derived_from<Types, IStreamable> && ...)>
{
};

template <typename> struct is_not_istreamable : std::true_type
{
};
template <typename... Types>
struct is_not_istreamable<std::tuple<Types...>>
    : std::integral_constant<bool, !(std::is_same_v<IStreamable, Types> || ...)>
{
};
} // namespace detail

template <typename Type> inline constexpr bool is_pair_v = detail::is_pair<Type>::value;
template <typename Type> inline constexpr bool is_tuple_v = detail::is_tuple<Type>::value;
template <typename Type> inline constexpr bool is_variant_v = detail::is_variant<Type>::value;
template <typename Type> inline constexpr bool is_optional_v = detail::is_optional<Type>::value;
template <typename Type> inline constexpr bool is_unique_ptr_v = detail::is_unique_ptr<Type>::value;
template <typename Type> inline constexpr bool is_shared_ptr_v = detail::is_shared_ptr<Type>::value;
template <typename Type> inline constexpr bool is_basic_string_v = detail::is_basic_string<Type>::value;
template <typename Type>
inline constexpr bool are_derived_from_istreamable_v = detail::are_derived_from_istreamable<Type>::value;
template <typename Type> inline constexpr bool is_not_istreamable_v = detail::is_not_istreamable<Type>::value;

template <typename> inline constexpr auto always_false = false;

template <typename Type>
concept is_wstring = std::is_same_v<typename Type::value_type, std::wstring::value_type> && is_basic_string_v<Type>;

template <typename Type>
concept is_u16string = std::is_same_v<typename Type::value_type, std::u16string::value_type> && is_basic_string_v<Type>;

template <typename Type>
concept is_utf16string = is_u16string<Type> || (sizeof(std::wstring::value_type) == 2 && is_wstring<Type>);

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

[[nodiscard]] consteval bool static_equal(const char *aString1, const char *aString2) noexcept
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
         - add separated examples
         - refactor tests

    FEATURES:
         - instead of reading object to jump over the value, create a jump method

    UX:
         - when finding derived class from base class pointer, add a tuple representing the types that can be read
   and make the user access the objects by index so can't read a bad object
*/
