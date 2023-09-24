#pragma once

#include "Stream.h"

namespace hbann
{
class IStreamable
{
    friend class StreamWriter;

  public:
    virtual std::string_view ToStream() = 0;

    virtual void FromStream(std::string_view aStream) = 0;

  protected:
    virtual constexpr size_range FindParseSize() const noexcept = 0;

  private:
    Stream mStream;
};
} // namespace hbann
