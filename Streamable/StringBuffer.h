#pragma once

namespace hbann
{
class StringBuffer : public std::stringbuf
{
  public:
    enum class State : uint8_t
    {
        NONE = 0b00,
        WRITE = 0b01,
        READ = 0b10,
        BOTH = 0b11
    };

    inline StringBuffer(const State aState = State::NONE) : mState(aState)
    {
    }

    constexpr bool Can(const State aState) const noexcept
    {
        if (mState == aState)
        {
            return true;
        }

        return (uint8_t)mState & (uint8_t)aState;
    }

  protected:
    State mState = State::NONE;

    inline std::stringbuf *setbuf(char_type *aData, std::streamsize aSize) noexcept override
    {
        if (aSize <= 0)
        {
            return this;
        }

        if (aData)
        {
            auto dataStart = aData;
            auto dataEnd = aData + aSize;

            setg(dataStart, dataStart, dataEnd);
            setp(dataStart, Can(State::WRITE) ? dataStart : dataEnd, dataEnd);
        }
        else
        {
            Clear();
            setbuf(new char_type[(size_t)aSize], aSize);
        }

        return this;
    }

  private:
    inline void Clear() noexcept
    {
        auto streamO(pbase());
        auto streamI(eback());
        if (!streamO && !streamI)
        {
            return;
        }

        // at least one pointer exists so delete at lease one
        // if the pointers were not the same delete the other one
        if (streamI == streamO)
        {
            delete streamI;
        }
        else if (streamI != streamO)
        {
            delete streamI;
            delete streamO;
        }

        // reset internal pointers
        setg(nullptr, nullptr, nullptr);
        setp(nullptr, nullptr, nullptr);
    }
};

} // namespace hbann

namespace std
{
[[nodiscard]] inline string to_string(const hbann::StringBuffer::State aState)
{
    switch (aState)
    {
    case hbann::StringBuffer::State::NONE:
        return "NONE";
    case hbann::StringBuffer::State::WRITE:
        return "WRITE";
    case hbann::StringBuffer::State::READ:
        return "READ";
    case hbann::StringBuffer::State::BOTH:
        return "BOTH";

    default:
        return "";
    }
}
} // namespace std
