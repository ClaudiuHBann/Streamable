#include "pch.h"
#include "StreamWriter.h"
#include "IStreamable.h"

namespace hbann
{
void StreamWriter::WriteStreamable(IStreamable &aStreamable)
{
    WriteObjectOfKnownSize(aStreamable.FindParseSize());

    const auto &streamView = aStreamable.ToStream().GetBuffer().view();
    mStream->Write(streamView.data(), (size_range)streamView.size());
}
} // namespace hbann
