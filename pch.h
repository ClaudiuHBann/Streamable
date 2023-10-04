#pragma once

namespace std::filesystem
{
class path;
} // namespace std::filesystem

// std
#ifdef __GNUC__
#include <cstdint>
#endif // __GNUC__
#include <sstream>

// Streamable
#define STREAMABLE_INTERFACE_NAME "IStreamable"

namespace hbann
{
using size_range = uint32_t;

template <typename> constexpr auto always_false = false;

template <typename Type> using get_raw_t = std::remove_const_t<std::remove_reference_t<Type>>;

template <typename Container>
concept has_method_reserve = requires(Container &aContainer) { aContainer.reserve(size_t(0)); };

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

template <std::ranges::range Range> constexpr size_t GetRangeCount(const Range &aRange)
{
    if constexpr (has_method_size<Range>)
    {
        return std::ranges::size(aRange);
    }
    else if constexpr (is_path<Range>)
    {
        return aRange.native().size();
    }
    else
    {
        static_assert(always_false<Range>, "Implement your own size getter bitch, sorry :(");
    }
}
} // namespace hbann

/*
    TODO:
         - create our own exception class for more specific info
         - throw an exception for any copy that is made for any of our classes (we just MOVE objects)
         - check the reserve method
*/
