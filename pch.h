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
concept is_pointer_ex = std::is_pointer_v<Type> || is_shared_ptr_v<Type> || is_unique_ptr_v<Type>;

template <typename Derived, typename Base>
concept is_derived_from_with_ptr =
    std::is_pointer_v<Derived> && std::derived_from<std::remove_pointer_t<Derived>, Base> ||
    (is_unique_ptr_v<Derived> && std::derived_from<typename Derived::element_type, Base>) ||
    (is_shared_ptr_v<Derived> && std::derived_from<typename Derived::element_type, Base>);

template <typename Type>
concept is_std_lay_no_ptr = std::is_standard_layout_v<Type> && !is_pointer_ex<Type>;

template <typename Type>
concept is_path = std::is_same_v<std::remove_cvref_t<Type>, std::filesystem::path>;

template <typename Container>
concept is_range_std_lay =
    (std::ranges::contiguous_range<Container> && is_std_lay_no_ptr<typename Container::value_type>) ||
    is_path<Container>;

template <typename Container>
concept has_method_size = requires(Container &aContainer) { std::ranges::size(aContainer); };

template <typename Class>
concept has_method_find_derived_streamable =
    std::derived_from<std::remove_pointer_t<std::decay_t<Class>>, IStreamable> &&
    requires(StreamReader &aStreamReader) {
        {
            Class::FindDerivedStreamable(aStreamReader)
        } -> std::convertible_to<IStreamable *>;
    };

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
         - ??? objects that have std_lay should come before like std::pair or std::tuple
         - ??? FindRangeSize should not check for contiguous range when finding size of a range
         - can Streamable call the intermediate class's FindDerivedStreamable automatically?
         - when reserving size for wstrings that have been encoded we reserve more (worst case x2)
         - check SizeFinder for added type support and for incorrect use of the Size::findrequired bytes and etc...
         - add separated examples
         - remake tests

    FEATURES:
         - remove raw pointer support and add support for unique_ptr and shared_ptr

    UX:
         - when finding derived class from base class pointer, add a tuple representing the types that can be read
   and make the user access the objects by index so can't read a bad object

    WARNING:
         - when adding a new feature try to implement the serialize and deserialize algorithms with SizeFinder for
   the object and, if possible, the reserve method
*/
