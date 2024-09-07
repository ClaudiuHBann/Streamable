#pragma once

TEST_CASE("Streamable::Independent::Utilities", "[Streamable][Independent][Utilities]")
{
    SECTION("SizeFinder")
    {
        int i{42};
        REQUIRE(hbann::SizeFinder::FindRangeRank<decltype(i)>() == 0);

        std::list<std::pair<int, float>> l{{22, 14.f}, {93, 32.f}};
        REQUIRE(hbann::SizeFinder::FindRangeRank<decltype(l)>() == 1);

        std::vector<double> v{512., 52., 77., 42321.};
        REQUIRE(hbann::SizeFinder::FindRangeRank<decltype(v)>() == 1);

        enum class enumClassTest : uint8_t
        {
            NONE,
            NOTHING,
            NADA
        };

        std::list<std::vector<enumClassTest>> lv{{enumClassTest::NONE, enumClassTest::NOTHING},
                                                 {enumClassTest::NOTHING, enumClassTest::NADA}};
        REQUIRE(hbann::SizeFinder::FindRangeRank<decltype(lv)>() == 2);

        size_t lvSize = 1;
        for (const auto &lvItem : lv)
        {
            lvSize += 1 + lvItem.size() * sizeof(decltype(lv)::value_type::value_type);
        }

        std::vector<std::vector<std::string>> vvs{{"gsbbbawf", "hbann", "1fwah10"}, {"palelica", "t43hachhew"}};
        REQUIRE(hbann::SizeFinder::FindRangeRank<decltype(vvs)>() == 3); // the string is a range itself

        size_t vvsSize = 1;
        for (const auto &vsItem : vvs)
        {
            vvsSize += 1;
            for (const auto &sItem : vsItem)
            {
                vvsSize += 1 + sItem.size() * sizeof(std::remove_cvref_t<decltype(sItem)>::value_type);
            }
        }
    }
}
