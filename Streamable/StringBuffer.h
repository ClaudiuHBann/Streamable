#pragma once

namespace hbann
{
class StringBuffer : public std::stringbuf
{
  protected:
    std::stringbuf *setbuf(char_type *aData, std::streamsize aSize)
    {
        if (aData && aSize > 0)
        {
            setg(aData, aData, aData + aSize);
            setp(aData, aData + aSize, aData + aSize);
        }
        else
        {
            _ASSERT(false && "Allocate buffer");
        }

        return this;
    }
};
} // namespace hbann
