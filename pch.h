#pragma once

namespace std::filesystem
{
class path;
} // namespace std::filesystem

namespace hbann
{
class IStreamable;
class StreamReader;
} // namespace hbann

// std
#include <bit>
#include <cmath>
#include <memory>
#include <span>
#include <variant>
#include <vector>

// Streamable
constexpr auto STREAMABLE_INTERFACE_NAME = "IStreamable";

namespace hbann
{
template <typename> constexpr auto always_false = false;

template <typename Container>
concept has_method_reserve =
    std::ranges::contiguous_range<Container> && requires(Container &aContainer) { aContainer.reserve(size_t(0)); };

template <typename Type>
concept is_pointer_unique = std::is_same_v<std::remove_cvref_t<Type>, std::unique_ptr<typename Type::value_type>>;

template <typename Type>
concept is_pointer_shared = std::is_same_v<std::remove_cvref_t<Type>, std::shared_ptr<typename Type::value_type>>;

template <typename Type>
concept is_pointer = std::is_pointer_v<Type> || is_pointer_unique<Type> || is_pointer_shared<Type>;

template <typename Type>
concept is_std_lay_no_ptr = std::is_standard_layout_v<Type> && !is_pointer<Type>;

template <typename Base, typename Derived>
concept is_base_of_no_ptr = std::is_base_of_v<Base, std::remove_pointer_t<Derived>>;

template <typename Type>
concept is_path = std::is_same_v<std::remove_cvref_t<Type>, std::filesystem::path>;

template <typename Container>
concept is_range_std_lay =
    (std::ranges::contiguous_range<Container> && is_std_lay_no_ptr<typename Container::value_type>) ||
    is_path<Container>;

template <typename Container>
concept has_method_size = requires(Container &aContainer) { std::ranges::size(aContainer); };

template <typename Class>
concept has_method_find_derived_streamable = requires(StreamReader &aStreamReader) {
    {
        Class::FindDerivedStreamable(aStreamReader)
    } -> std::convertible_to<IStreamable *>;
};

constexpr bool static_equal(const char *aString1, const char *aString2) noexcept
{
    return *aString1 == *aString2 && (!*aString1 || static_equal(aString1 + 1, aString2 + 1));
}
} // namespace hbann

/*
    TODO:
         - when finding derived class from base class pointer, add a tuple representing the types that can be read and
        make the user access the objects by index so can't read a bad object
         - add separated examples
         - add support for unique_ptr and shared_ptr
         - encode string in utf-8 to save space
         - FindRangeSize should not check for contiguous range when finding size of a range
         - don't add a 4 byte for every range add a byte if the most significant bit is true than we have a full 4 byte
        else we have just a byte
*/
