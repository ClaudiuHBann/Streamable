#pragma once

#include "Stream.h"

namespace hbann
{
class IStreamable
{
    friend class StreamWriter;
    friend class StreamReader;

  public:
    virtual Stream &ToStream()
    {
        return mStream;
    }

  protected:
    virtual constexpr size_t FindParseSize() const noexcept
    {
        return 0;
    }

  private:
    Stream mStream;
};
} // namespace hbann
