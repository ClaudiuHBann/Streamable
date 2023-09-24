#include "pch.h"
#include "StreamReader.h"
#include "IStreamable.h"

namespace hbann
{
template <typename Type> [[nodiscard]] constexpr decltype(auto) StreamReader::ReadStreamable() noexcept
{
    static_assert(std::is_base_of_v<IStreamable, Type>, "Type is not a streamable!");

    Type streamable{};
    streamable.FromStream(mStream.Read(ReadObjectOfKnownSize<size_range>()));
    return streamable;
}
} // namespace hbann
