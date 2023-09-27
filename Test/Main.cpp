#include "pch.h"

TEST_CASE("Streamable", "[Streamable]")
{
    SECTION("SizeFinder")
    {
        int i{42};
        REQUIRE(SizeFinder::FindRangeRank<decltype(i)>() == 0);
        REQUIRE(SizeFinder::FindParseSize(i) == (size_range)sizeof(i));

        std::list<std::pair<int, float>> l{{22, 14.f}, {93, 32.f}};
        REQUIRE(SizeFinder::FindRangeRank<decltype(l)>() == 1);
        REQUIRE(SizeFinder::FindParseSize(l) ==
                size_range(sizeof(size_range) + l.size() * sizeof(decltype(l)::value_type)));

        std::vector<double> v{512., 52., 77., 42321.};
        REQUIRE(SizeFinder::FindRangeRank<decltype(v)>() == 1);
        REQUIRE(SizeFinder::FindParseSize(v) ==
                size_range(sizeof(size_range) + v.size() * sizeof(decltype(v)::value_type)));

        enum class enumClassTest : uint8_t
        {
            NONE,
            NOTHING,
            NADA
        };

        std::list<std::vector<enumClassTest>> lv{{enumClassTest::NONE, enumClassTest::NOTHING},
                                                 {enumClassTest::NOTHING, enumClassTest::NADA}};
        REQUIRE(SizeFinder::FindRangeRank<decltype(lv)>() == 2);

        size_t lvSize = sizeof(size_range);
        for (const auto &lvItem : lv)
        {
            lvSize += sizeof(size_range) + lvItem.size() * sizeof(decltype(lv)::value_type::value_type);
        }
        REQUIRE(SizeFinder::FindParseSize(lv) == (size_range)lvSize);

        std::vector<std::vector<std::string>> vvs{{"gsbbbawf", "hbann", "1fwah10"}, {"palelica", "t43hachhew"}};
        REQUIRE(SizeFinder::FindRangeRank<decltype(vvs)>() == 3); // the string is a range itself

        size_t vvsSize = sizeof(size_range);
        for (const auto &vsItem : vvs)
        {
            vvsSize += sizeof(size_range);
            for (const auto &sItem : vsItem)
            {
                vvsSize += sizeof(size_range) + sItem.size() * sizeof(get_raw_t<decltype(sItem)>::value_type);
            }
        }
        REQUIRE(SizeFinder::FindParseSize(vvs) == (size_range)vvsSize);

        // TODO: add IStreamable SizeFinder test
    }

    SECTION("Stream")
    {
        Stream stream;
        stream.Reserve(21);

        std::string biceps("biceps");
        stream.Write(biceps.c_str(), (size_range)biceps.size()).Flush();
        const auto bicepsView = stream.Read((size_range)biceps.size());
        REQUIRE(biceps.compare(bicepsView) == 0);

        REQUIRE(!stream.Read(1).size());

        std::string triceps("triceps");
        stream.Write(triceps.c_str(), (size_range)triceps.size()).Flush();
        const auto tricepsView = stream.Read((size_range)triceps.size());
        REQUIRE(triceps.compare(tricepsView) == 0);

        REQUIRE(!stream.Read(1).size());

        std::string cariceps("cariceps");
        stream.Write(cariceps.c_str(), (size_range)cariceps.size()).Flush();
        const auto caricepsView = stream.Read((size_range)cariceps.size());
        REQUIRE(cariceps.compare(caricepsView) == 0);
    }

    SECTION("StreamWriter")
    {
        Stream stream;
        StreamWriter streamWriter(stream);

        double d = 12.34;
        std::string s("cariceps");
        streamWriter.WriteAll(d, s);

        const auto dView = stream.Read(sizeof(d)).data();
        REQUIRE(d == *reinterpret_cast<const decltype(d) *>(dView));

        const auto sSizeView = stream.Read(sizeof(size_range)).data();
        const auto sSize = *reinterpret_cast<const size_range *>(sSizeView);
        REQUIRE(s.size() == sSize);
        const auto sView = stream.Read(sSize);
        REQUIRE(s.compare(sView) == 0);

        // TODO: add IStreamable StreamWriter test
    }

    SECTION("StreamReader")
    {
        Stream stream;
        StreamWriter streamWriter(stream);
        StreamReader streamReader(stream);

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

class Shape : public IStreamable
{
    STREAMABLE_DEFINE(IStreamable, mType);

  protected:
    [[nodiscard]] IStreamable *FindDerivedStreamable(StreamReader &aStreamReader) override;

  public:
    enum class Type : uint8_t
    {
        UNKNOWN,
        RECTANGLE,
        SQUARE,
        CIRCLE
    };

    constexpr Shape() noexcept = default;

    Shape(const Type &aType) : mType(aType)
    {
    }

    bool operator==(const Shape &aShape) const
    {
        return mType == aShape.mType;
    }

    bool operator!=(const Shape &aShape) const
    {
        return !(*this == aShape);
    }

    const Type &GetType() const
    {
        return mType;
    }

  private:
    Type mType = Type::UNKNOWN;
};

class Circle : public Shape
{
    STREAMABLE_DEFINE(Shape, mRadius);

  public:
    Circle() : Shape(Type::CIRCLE)
    {
    }

    Circle(const double aRadius) : Shape(Type::CIRCLE), mRadius(aRadius)
    {
    }

    bool operator==(const Circle &aCircle) const
    {
        return *(Shape *)this == *(Shape *)&aCircle && mRadius == aCircle.mRadius;
    }

    bool operator!=(const Circle &aCircle) const
    {
        return !(*this == aCircle);
    }

  private:
    double mRadius{};
};

[[nodiscard]] IStreamable *Shape::FindDerivedStreamable(StreamReader &aStreamReader)
{
    Shape::Type type{};
    aStreamReader.ReadAll(type);

    switch (type)
    {
    case Shape::Type::CIRCLE:
        return new Circle;

    default:
        return nullptr;
    }
}

TEST_CASE("IStreamable", "[IStreamable]")
{
    SECTION("Simple")
    {
        class Something : public IStreamable
        {
            STREAMABLE_DEFINE(IStreamable, mNickname, mIDK);

          public:
            Something() noexcept = default;

            Something(const std::wstring &aNickname, const size_t aAge) : mNickname(aNickname), mIDK(aAge)
            {
            }

            bool operator==(const Something &aSomething) const
            {
                return mNickname == aSomething.mNickname && mIDK == aSomething.mIDK;
            }

          private:
            std::wstring mNickname{};
            size_t mIDK{};
        };

        Something smth(L"HBann", 1234567890);
        Something smthElse{};
        [[maybe_unused]] auto _(smthElse.FromStream(smth.ToStream()));

        REQUIRE(smth == smthElse);
    }

    SECTION("Derived")
    {
        Shape *circleStart = new Circle(3.14156);
        Shape *circleEnd = new Circle(2.4);
        [[maybe_unused]] auto _(circleEnd->FromStream(circleStart->ToStream()));

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
        [[maybe_unused]] auto _(sphereEnd.FromStream(sphereStart.ToStream()));

        REQUIRE(sphereStart == sphereEnd);
    }

    SECTION("BaseClass*")
    {
        class Context : public IStreamable
        {
            STREAMABLE_DEFINE(IStreamable, mShapes);

          public:
            constexpr Context() noexcept = default;

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
        [[maybe_unused]] auto _(contextEnd.FromStream(contextStart.ToStream()));

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

// class == ""s -> use strcmp_s
// move -> std::move
// setp only has 2 params use the one with 2 params and offset the next pointer
