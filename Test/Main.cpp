#include "pch.h"

typedef struct _guid
{
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
} guid;

constexpr guid GUID_RND = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

class Shape : public hbann::IStreamable
{
    STREAMABLE_DEFINE(IStreamable, mType, mID);

  public:
    enum class Type : uint8_t
    {
        NONE,
        CIRCLE,
        RECTANGLE
    };

    Shape() = default;
    Shape(const Type aType, const guid &aID) : mType(aType), mID(aID)
    {
    }

    virtual ~Shape() = default;

    bool operator==(const Shape &aShape) const
    {
        return mType == aShape.mType && !memcmp(&mID, &aShape.mID, sizeof(mID));
    }

    Type GetType()
    {
        return mType;
    }

  protected:
    hbann::IStreamable *FindDerivedStreamable(hbann::StreamReader &aStreamReader) override;

  private:
    Type mType = Type::NONE;
    guid mID{};
};

class Circle : public Shape
{
    STREAMABLE_DEFINE(Shape, mSVG, mURL);

  public:
    Circle() = default;
    Circle(const guid &aID, const std::string &aSVG, const std::filesystem::path &aURL)
        : Shape(Type::CIRCLE, aID), mSVG(aSVG), mURL(aURL)
    {
    }

    bool operator==(const Circle &aCircle) const
    {
        return *(Shape *)this == *(Shape *)&aCircle && mSVG == aCircle.mSVG && mURL == aCircle.mURL;
    }

  private:
    std::string mSVG{};
    std::filesystem::path mURL{};
};

class Sphere : public Circle
{
    STREAMABLE_DEFINE(Circle, mReflexion);

  public:
    Sphere() = default;
    Sphere(const Circle &aCircle, const bool aReflexion) : Circle(aCircle), mReflexion(aReflexion)
    {
    }

    bool operator==(const Sphere &aSphere) const
    {
        return *(Circle *)this == *(Circle *)&aSphere && mReflexion == aSphere.mReflexion;
    }

  private:
    bool mReflexion{};
};

class Rectangle : public Shape
{
    STREAMABLE_DEFINE(Shape, mCenter, mCells);

  public:
    Rectangle() = default;
    Rectangle(const guid &aID, const Sphere &aCenter, const std::vector<std::vector<std::wstring>> &aCells)
        : Shape(Type::RECTANGLE, aID), mCenter(aCenter), mCells(aCells)
    {
    }

    bool operator==(const Rectangle &aRectangle) const
    {
        return *(Shape *)this == *(Shape *)&aRectangle && mCenter == aRectangle.mCenter && mCells == aRectangle.mCells;
    }

  private:
    Sphere mCenter{};
    std::vector<std::vector<std::wstring>> mCells{};
};

hbann::IStreamable *Shape::FindDerivedStreamable(hbann::StreamReader &aStreamReader)
{
    Type type{};
    aStreamReader.ReadAll(type);

    switch (type)
    {
    case Shape::Type::CIRCLE:
        return new Circle;
    case Shape::Type::RECTANGLE:
        return new Rectangle;

    default:
        return nullptr;
    }
}

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
        stream.Write(biceps.c_str(), biceps.size());
        const auto bicepsView = stream.Read(biceps.size());
        REQUIRE(biceps.compare(0, biceps.size(), bicepsView.data(), bicepsView.size()) == 0);

        REQUIRE(!stream.Read(1).size());

        std::string triceps("triceps");
        stream.Write(triceps.c_str(), triceps.size());
        const auto tricepsView = stream.Read(triceps.size());
        REQUIRE(triceps.compare(0, triceps.size(), tricepsView.data(), tricepsView.size()) == 0);

        REQUIRE(!stream.Read(1).size());

        std::string cariceps("cariceps");
        stream.Write(cariceps.c_str(), cariceps.size());
        const auto caricepsView = stream.Read(cariceps.size());
        REQUIRE(cariceps.compare(0, cariceps.size(), caricepsView.data(), caricepsView.size()) == 0);
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
        REQUIRE(s.compare(0, s.size(), sView.data(), sView.size()) == 0);

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
        Shape shapeStart(Shape::Type::RECTANGLE, GUID_RND);
        Shape shapeEnd{};
        shapeEnd.Deserialize(shapeStart.Serialize());

        REQUIRE(shapeStart == shapeEnd);
    }

    SECTION("Derived")
    {
        Shape *circleStart = new Circle(GUID_RND, "SVG", L"URL\\SHIT");
        Shape *circleEnd = new Circle();
        circleEnd->Deserialize(circleStart->Serialize());

        REQUIRE(*(Circle *)circleStart == *(Circle *)circleEnd);
    }

    SECTION("Derived++")
    {
        Sphere sphereStart({GUID_RND, "SVG", L"URL\\SHIT"}, true);
        Sphere sphereEnd;
        sphereEnd.Deserialize(sphereStart.Serialize());

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
                    if (mShapes[i]->GetType() != aContext.mShapes[i]->GetType())
                    {
                        return false;
                    }

                    switch (mShapes[i]->GetType())
                    {
                    case Shape::Type::CIRCLE: {
                        if (*(Circle *)mShapes[i] != *(Circle *)aContext.mShapes[i])
                        {
                            return false;
                        }

                        break;
                    }

                    case Shape::Type::RECTANGLE: {
                        if (*(Rectangle *)mShapes[i] != *(Rectangle *)aContext.mShapes[i])
                        {
                            return false;
                        }

                        break;
                    }
                    }
                }

                return true;
            }

          private:
            std::vector<Shape *> mShapes{};
        };

        Sphere center({GUID_RND, "SVG", L"URL\\SHIT"}, true);
        std::vector<std::vector<std::wstring>> cells{{L"smth", L"else"}, {L"HBann", L"Sefu la bani"}};
        std::vector<Shape *> shapes{new Circle(GUID_RND, "Circle1_SVG", "Circle1_URL"),
                                    new Rectangle(GUID_RND, center, {}),
                                    new Circle(GUID_RND, "Circle2_SVG", "Circle2_URL")};
        Context contextStart(std::move(shapes));

        Context contextEnd;
        contextEnd.Deserialize(contextStart.Serialize());

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
