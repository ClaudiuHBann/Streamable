#include "pch.h"

typedef struct _guid
{
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
} guid;

std::string to_string(const guid &aGUID)
{
    char buffer[40]{};
    sprintf_s(buffer, "{%08x-%04hx-%04hx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx}", aGUID.Data1, aGUID.Data2,
              aGUID.Data3, aGUID.Data4[0], aGUID.Data4[1], aGUID.Data4[2], aGUID.Data4[3], aGUID.Data4[4],
              aGUID.Data4[5], aGUID.Data4[6], aGUID.Data4[7]);
    return buffer;
}

guid from_string(const std::string &aGUID)
{
    guid guid{};
    sscanf_s(aGUID.c_str(), "{%08x-%04hx-%04hx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx}", &guid.Data1,
             &guid.Data2, &guid.Data3, &guid.Data4[0], &guid.Data4[1], &guid.Data4[2], &guid.Data4[3], &guid.Data4[4],
             &guid.Data4[5], &guid.Data4[6], &guid.Data4[7]);
    return guid;
}

class Shape : public hbann::IStreamable
{

  public:
    enum class Type : uint8_t
    {
        NONE,
        CIRCLE,
        RECTANGLE
    };

  protected:
    Shape() = default;
    Shape(const Type aType) : mType(aType)
    {
    }

    virtual ~Shape() = default;

    IStreamable *FindDerivedStreamable(hbann::StreamReader &aStreamReader) override;

  private:
    Type mType = Type::NONE;
    guid mID{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    STREAMABLE_DEFINE(IStreamable, mType, mID);
};

class Circle : public Shape
{
    std::string mSVG{};
    std::filesystem::path mURL{};

  public:
    Circle() : Shape(Type::CIRCLE)
    {
        mSVG.resize(1'000'000);
        for (size_t i = 0; i < mSVG.size(); i++)
        {
            mSVG[i] = (char)i;
        }
    }

    STREAMABLE_DEFINE(Shape, mSVG, mURL);
};

class Rectangle : public Shape
{
    STREAMABLE_DEFINE(Shape, mCells, mCenter);

  public:
    Rectangle() : Shape(Type::RECTANGLE)
    {
        mCells.resize(100, std::vector(100, std::wstring()));
        for (size_t i = 0; i < mCells.size(); i++)
        {
            for (size_t j = 0; j < mCells[i].size(); j++)
            {
                mCells[i][j] = L"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
                               L"tempor incididunt ut "
                               L"labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud "
                               L"exercitation ullamco "
                               L"laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in "
                               L"reprehenderit in "
                               L"voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint "
                               L"occaecat cupidatat "
                               L"non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";
            }
        }
    }

  private:
    Circle mCenter{};
    std::vector<std::vector<std::wstring>> mCells{};
};

TEST_CASE("Streamable", "[Streamable]")
{
    SECTION("SizeFinder")
    {
        int i{42};
        REQUIRE(hbann::SizeFinder::FindRangeRank<decltype(i)>() == 0);
        REQUIRE(hbann::SizeFinder::FindParseSize(i) == sizeof(i));

        std::list<std::pair<int, float>> l{{22, 14.f}, {93, 32.f}};
        REQUIRE(hbann::SizeFinder::FindRangeRank<decltype(l)>() == 1);
        REQUIRE(hbann::SizeFinder::FindParseSize(l) ==
                sizeof(hbann::size_range) + l.size() * sizeof(decltype(l)::value_type));

        std::vector<double> v{512., 52., 77., 42321.};
        REQUIRE(hbann::SizeFinder::FindRangeRank<decltype(v)>() == 1);
        REQUIRE(hbann::SizeFinder::FindParseSize(v) ==
                sizeof(hbann::size_range) + v.size() * sizeof(decltype(v)::value_type));

        enum class enumClassTest : uint8_t
        {
            NONE,
            NOTHING,
            NADA
        };

        std::list<std::vector<enumClassTest>> lv{{enumClassTest::NONE, enumClassTest::NOTHING},
                                                 {enumClassTest::NOTHING, enumClassTest::NADA}};
        REQUIRE(hbann::SizeFinder::FindRangeRank<decltype(lv)>() == 2);

        size_t lvSize = sizeof(hbann::size_range);
        for (const auto &lvItem : lv)
        {
            lvSize += sizeof(hbann::size_range) + lvItem.size() * sizeof(decltype(lv)::value_type::value_type);
        }
        REQUIRE(hbann::SizeFinder::FindParseSize(lv) == lvSize);

        std::vector<std::vector<std::string>> vvs{{"gsbbbawf", "hbann", "1fwah10"}, {"palelica", "t43hachhew"}};
        REQUIRE(hbann::SizeFinder::FindRangeRank<decltype(vvs)>() == 3); // the string is a range itself

        size_t vvsSize = sizeof(hbann::size_range);
        for (const auto &vsItem : vvs)
        {
            vvsSize += sizeof(hbann::size_range);
            for (const auto &sItem : vsItem)
            {
                vvsSize +=
                    sizeof(hbann::size_range) + sItem.size() * sizeof(hbann::get_raw_t<decltype(sItem)>::value_type);
            }
        }
        REQUIRE(hbann::SizeFinder::FindParseSize(vvs) == vvsSize);

        // TODO: add IStreamable SizeFinder test
    }

    SECTION("Stream")
    {
        hbann::Stream stream;
        stream.Reserve(21);

        std::string biceps("biceps");
        stream.Write(biceps.c_str(), biceps.size()).Flush();
        const auto bicepsView = stream.Read(biceps.size());
        REQUIRE(biceps.compare(bicepsView) == 0);

        REQUIRE(!stream.Read(1).size());

        std::string triceps("triceps");
        stream.Write(triceps.c_str(), triceps.size()).Flush();
        const auto tricepsView = stream.Read(triceps.size());
        REQUIRE(triceps.compare(tricepsView) == 0);

        REQUIRE(!stream.Read(1).size());

        std::string cariceps("cariceps");
        stream.Write(cariceps.c_str(), cariceps.size()).Flush();
        const auto caricepsView = stream.Read(cariceps.size());
        REQUIRE(cariceps.compare(caricepsView) == 0);
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

        const auto sSizeView = stream.Read(sizeof(hbann::size_range)).data();
        const auto sSize = *reinterpret_cast<const hbann::size_range *>(sSizeView);
        REQUIRE(s.size() == sSize);
        const auto sView = stream.Read(sSize);
        REQUIRE(s.compare(sView) == 0);

        // TODO: add IStreamable StreamWriter test
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

        // TODO: add IStreamable StreamReader test
    }
}

TEST_CASE("IStreamable", "[IStreamable]")
{
    SECTION("Simple")
    {
        Shape smth(Shape::Type::SQUARE);
        Shape smthElse{};
        DISCARD(smthElse.FromStream(smth.ToStream()));

        REQUIRE(smth == smthElse);
    }

    SECTION("Derived")
    {
        Shape *circleStart = new Circle(3.14156);
        Shape *circleEnd = new Circle(2.4);
        DISCARD(circleEnd->FromStream(circleStart->ToStream()));

        REQUIRE(*(Circle *)circleStart == *(Circle *)circleEnd);
    }

    SECTION("Derived++")
    {
        class Sphere : public Circle
        {
            STREAMABLE_DEFINE(Circle, mReflexion);

          public:
            Sphere() : Circle()
            {
            }

            Sphere(const double aRadius, const bool aReflexion) : Circle(aRadius), mReflexion(aReflexion)
            {
            }

            bool operator==(const Sphere &aSphere) const
            {
                return *(Circle *)this == *(Circle *)&aSphere && mReflexion == aSphere.mReflexion;
            }

          private:
            bool mReflexion{};
        };

        Sphere sphereStart(3.14156, true);
        Sphere sphereEnd;
        DISCARD(sphereEnd.FromStream(sphereStart.ToStream()));

        REQUIRE(sphereStart == sphereEnd);
    }

    SECTION("BaseClass*")
    {
        class Context : public hbann::IStreamable
        {
            STREAMABLE_DEFINE(IStreamable, mShapes);

          public:
            Context() = default;

            Context(std::vector<Shape *> &&aShapes) : mShapes(std::move(aShapes))
            {
            }

            bool operator==(const Context &aContext) const
            {
                if (mShapes.size() != aContext.mShapes.size())
                {
                    return false;
                }

                for (size_t i = 0; i < mShapes.size(); i++)
                {
                    if (*mShapes[i] != *aContext.mShapes[i])
                    {
                        return false;
                    }

                    if (*(Circle *)mShapes[i] != *(Circle *)aContext.mShapes[i])
                    {
                        return false;
                    }
                }

                return true;
            }

          private:
            std::vector<Shape *> mShapes{};
        };

        std::vector<Shape *> shapes{new Circle(3.14156), new Circle(2.4)};
        Context contextStart(std::move(shapes));

        Context contextEnd;
        DISCARD(contextEnd.FromStream(contextStart.ToStream()));

        REQUIRE(contextStart == contextEnd);
    }
}

int main(int argc, char **argv)
{
    int returnCode{};
    {
        Session session;

#if _DEBUG
        ConfigData configData{.showSuccessfulTests = true,
                              .shouldDebugBreak = true,
                              .noThrow = true,
                              .showInvisibles = true,
                              .filenamesAsTags = true,

                              .verbosity = Verbosity::High,
                              .warnings =
                                  WarnAbout::What(WarnAbout::What::NoAssertions | WarnAbout::What::UnmatchedTestSpec),
                              .showDurations = ShowDurations::Always,
                              .defaultColourMode = ColourMode::PlatformDefault};

        session.useConfigData(configData);
#endif // _DEBUG

        returnCode = session.applyCommandLine(argc, argv);
        if (!returnCode)
        {
            returnCode = session.run();
        }
    }

    return returnCode;
}
