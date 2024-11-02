#pragma once

TEST_CASE("Streamable::Independent::Streams", "[Streamable][Independent][Streams]")
{
    SECTION("Stream")
    {
        hbann::Stream stream;

        std::string biceps("biceps");
        stream.Write({reinterpret_cast<uint8_t *>(biceps.data()), biceps.size()});
        const auto bicepsView = stream.Read(biceps.size());
        REQUIRE(!std::memcmp(biceps.c_str(), bicepsView.data(), bicepsView.size()));

        std::string triceps("triceps");
        stream.Write({reinterpret_cast<uint8_t *>(triceps.data()), triceps.size()});
        const auto tricepsView = stream.Read(triceps.size());
        REQUIRE(!std::memcmp(triceps.c_str(), tricepsView.data(), tricepsView.size()));

        std::string cariceps("cariceps");
        stream.Write({reinterpret_cast<uint8_t *>(cariceps.data()), cariceps.size()});
        const auto caricepsView = stream.Read(cariceps.size());
        REQUIRE(!std::memcmp(cariceps.c_str(), caricepsView.data(), caricepsView.size()));
    }

    SECTION("StreamWriter")
    {
        hbann::Stream stream;
        hbann::StreamWriter streamWriter(stream);

        double d = 12.34;
        std::string s("cariceps");
        streamWriter.WriteAll(d, s);

        const auto dView = stream.Read(sizeof(d)).data();
        REQUIRE(d == *reinterpret_cast<const decltype(d) *>(dView));

        const auto requiredBytes = hbann::Size::FindRequiredBytes(stream.Current());
        const auto sSize = hbann::Size::MakeSize(stream.Read(requiredBytes));
        REQUIRE(s.size() == sSize);

        const auto sView = stream.Read(sSize);
        REQUIRE(!std::memcmp(s.c_str(), sView.data(), sView.size()));
    }

    SECTION("StreamReader")
    {
        hbann::Stream stream;
        hbann::StreamWriter streamWriter(stream);
        hbann::StreamReader streamReader(stream);

        double d = 12.34;
        std::string s("cariceps");
        streamWriter.WriteAll(d, s);

        double dd{};
        std::string ss{};
        streamReader.ReadAll(dd, ss);

        REQUIRE(d == dd);
        REQUIRE(s == ss);
    }
}
