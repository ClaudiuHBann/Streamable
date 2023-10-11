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
#include <span>
#include <variant>
#include <vector>

// Streamable
constexpr auto STREAMABLE_INTERFACE_NAME = "IStreamable";

namespace hbann
{
using size_range = uint32_t;

template <typename> constexpr auto always_false = false;

template <typename Type> using get_raw_t = std::remove_const_t<std::remove_reference_t<Type>>;

template <typename Container>
concept has_method_reserve =
    std::ranges::contiguous_range<Container> && requires(Container &aContainer) { aContainer.reserve(size_t(0)); };

template <typename Type>
concept is_std_lay_no_ptr = std::is_standard_layout_v<Type> && !std::is_pointer_v<Type>;

template <typename Base, typename Derived>
concept is_base_of_no_ptr = std::is_base_of_v<Base, std::remove_pointer_t<Derived>>;

template <typename Type>
concept is_path = std::is_same_v<get_raw_t<Type>, std::filesystem::path>;

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

constexpr bool static_equal(char const *aString1, char const *aString2) noexcept
{
    return *aString1 == *aString2 && (!*aString1 || static_equal(aString1 + 1, aString2 + 1));
}
} // namespace hbann

/*
    TODO:
         - when finding derived class from base class pointer, add a tuple representing the types that can be read and
        make the user access the objects by index so can't read a bad object
*/
