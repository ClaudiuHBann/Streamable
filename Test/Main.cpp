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

  public:
    static hbann::IStreamable *FindDerivedStreamable(hbann::StreamReader &aStreamReader);

  private:
    Type mType = Type::NONE;
    guid mID{};
};

class Circle : public Shape
{
    STREAMABLE_DEFINE(Shape, mSVG, mURL, mVariant);

  public:
    Circle() = default;
    Circle(const guid &aID, const std::optional<std::string> &aSVG, const std::filesystem::path &aURL,
           std::variant<std::vector<double>, bool> &&aVariant)
        : Shape(Type::CIRCLE, aID), mSVG(aSVG), mURL(aURL), mVariant(aVariant)
    {
    }

    bool operator==(const Circle &aCircle) const
    {
        return *(Shape *)this == *(Shape *)&aCircle && mSVG == aCircle.mSVG && mURL == aCircle.mURL &&
               mVariant == aCircle.mVariant;
    }

  private:
    std::optional<std::string> mSVG{};
    std::filesystem::path mURL{};
    std::variant<std::vector<double>, bool> mVariant{};
};

class Sphere : public Circle
{
    STREAMABLE_DEFINE(Circle, mReflexion, mTuple, mPair);

  public:
    Sphere() = default;
    Sphere(const Circle &aCircle, const bool aReflexion, std::tuple<std::string, std::list<int>> &&aTuple,
           std::pair<Circle, double> &&aPair)
        : Circle(aCircle), mReflexion(aReflexion), mTuple(std::move(aTuple)), mPair(std::move(aPair))
    {
    }

    bool operator==(const Sphere &aSphere) const
    {
        return *(Circle *)this == *(Circle *)&aSphere && mReflexion == aSphere.mReflexion && mTuple == aSphere.mTuple &&
               mPair == aSphere.mPair;
    }

  private:
    bool mReflexion{};
    std::tuple<std::string, std::list<int>> mTuple{};
    std::pair<Circle, double> mPair{};
};

class RectangleEx : public Shape
{
    STREAMABLE_DEFINE(Shape, mCenter, mCells);

  public:
    RectangleEx() = default;
    RectangleEx(const guid &aID, const Sphere &aCenter, const std::vector<std::vector<std::wstring>> &aCells)
        : Shape(Type::RECTANGLE, aID), mCenter(aCenter), mCells(aCells)
    {
    }

    bool operator==(const RectangleEx &aRectangle) const
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
        return new RectangleEx;

    default:
        return nullptr;
    }
}

class Context : public hbann::IStreamable
{
    STREAMABLE_DEFINE(IStreamable, mShapes);

  public:
    Context() = default;
    Context(std::vector<Shape *> &&aShapes) : mShapes(std::move(aShapes))
    {
    }

    ~Context()
    {
        for (auto &shape : mShapes)
        {
            delete (shape);
        }
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
                if (*(RectangleEx *)mShapes[i] != *(RectangleEx *)aContext.mShapes[i])
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

TEST_CASE("Streamable", "[Streamable]")
{
    SECTION("SizeFinder")
    {
        int i{42};
        REQUIRE(hbann::SizeFinder::FindRangeRank<decltype(i)>() == 0);
        REQUIRE(hbann::SizeFinder::FindParseSize(i) == sizeof(i));

        std::list<std::pair<int, float>> l{{22, 14.f}, {93, 32.f}};
        REQUIRE(hbann::SizeFinder::FindRangeRank<decltype(l)>() == 1);
        REQUIRE(hbann::SizeFinder::FindParseSize(l) == 1 + l.size() * sizeof(decltype(l)::value_type));

        std::vector<double> v{512., 52., 77., 42321.};
        REQUIRE(hbann::SizeFinder::FindRangeRank<decltype(v)>() == 1);
        REQUIRE(hbann::SizeFinder::FindParseSize(v) == 1 + v.size() * sizeof(decltype(v)::value_type));

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
        REQUIRE(hbann::SizeFinder::FindParseSize(lv) == lvSize);

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
        REQUIRE(hbann::SizeFinder::FindParseSize(vvs) == vvsSize);

        // TODO: add IStreamable SizeFinder test
    }

    SECTION("Stream")
    {
        hbann::Stream stream;
        stream.Reserve(21);

        std::string biceps("biceps");
        stream.Write({reinterpret_cast<const uint8_t *>(biceps.c_str()), biceps.size()});
        const auto bicepsView = stream.Read(biceps.size());
        REQUIRE(!std::memcmp(biceps.c_str(), bicepsView.data(), bicepsView.size()));

        REQUIRE(!stream.Read(1).size());

        std::string triceps("triceps");
        stream.Write({reinterpret_cast<const uint8_t *>(triceps.c_str()), triceps.size()});
        const auto tricepsView = stream.Read(triceps.size());
        REQUIRE(!std::memcmp(triceps.c_str(), tricepsView.data(), tricepsView.size()));

        REQUIRE(!stream.Read(1).size());

        std::string cariceps("cariceps");
        stream.Write({reinterpret_cast<const uint8_t *>(cariceps.c_str()), cariceps.size()});
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
        Shape *circleStart = new Circle(GUID_RND, std::nullopt, L"URL\\SHIT", true);
        Shape *circleEnd = new Circle();
        circleEnd->Deserialize(circleStart->Serialize());

        REQUIRE(*(Circle *)circleStart == *(Circle *)circleEnd);

        delete circleStart;
        delete circleEnd;
    }

    SECTION("Derived++")
    {
        Circle circle(GUID_RND, "SVG", L"URL\\SHIT", std::vector{69., 420.});
        Sphere sphereStart(circle, true, {"Commit: added tuple support", {22, 100}}, {circle, 22.});
        Sphere sphereEnd;
        sphereEnd.Deserialize(sphereStart.Serialize());

        REQUIRE(sphereStart == sphereEnd);
    }

    SECTION("BaseClass*")
    {
        Circle circle(GUID_RND, {}, L"URL\\SHIT", false);
        Sphere center(circle, true, {"Commit: added tuple support", {22, 100}}, {circle, 22.});
        std::vector<std::vector<std::wstring>> cells{{L"smth", L"else"}, {L"HBann", L"Sefu la bani"}};
        std::vector<Shape *> shapes{new Circle(GUID_RND, "Circle1_SVG", "Circle1_URL", true),
                                    new RectangleEx(GUID_RND, center, cells),
                                    new Circle(GUID_RND, "Circle2_SVG", "Circle2_URL", std::vector{420., 69.})};
        ::Context contextStart(std::move(shapes));

        ::Context contextEnd;
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
        ConfigData configData{.showSuccessfulTests = false,
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
