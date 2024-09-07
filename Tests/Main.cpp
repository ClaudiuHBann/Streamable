#include "pch.h"
#include "Utilities/Classes.h"

#include "Independent/Streams.h"
#include "Independent/Utilities.h"

#include "Compatibility/Backwards.h"
#include "Compatibility/Forwards.h"

TEST_CASE("IStreamable", "[IStreamable]")
{
    SECTION("Simple")
    {
        Circle circleStart(GUID_RND, {}, L"URL\\SHIT", false);
        Circle circleEnd;
        circleEnd.Deserialize(circleStart.Serialize());
        circleStart.Deserialize(circleEnd.Serialize());

        REQUIRE(circleStart == circleEnd);
    }

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

    SECTION("DerivedxN")
    {
        Circle circle(GUID_RND, {}, L"URL\\SHIT", false);
        std::vector<std::vector<std::wstring>> cells{{L"smth", L"else"}, {L"HBann", L"Sefu la bani"}};

        Sphere sphere(circle, std::make_unique<bool>(true), {"Commit: added tuple support", {22, 100}}, {circle, 22.});

        Diamond diamondStart(GUID_RND, {}, L"URL\\SHIT", false, GUID_RND, std::move(sphere), cells);

        Diamond diamondEnd;
        diamondEnd.Deserialize(diamondStart.Serialize());

        REQUIRE(diamondStart == diamondEnd);
    }

    SECTION("Derived++")
    {
        Circle circle(GUID_RND, "SVG", L"URL\\SHIT", std::vector{69., 420.});
        Sphere sphereStart(circle, std::make_unique<bool>(true), {"Commit: added tuple support", {22, 100}},
                           {circle, 22.});
        Sphere sphereEnd;
        sphereEnd.Deserialize(sphereStart.Serialize());

        REQUIRE(sphereStart == sphereEnd);
    }

    SECTION("BaseClass*")
    {
        Circle circle(GUID_RND, {}, L"URL\\SHIT", false);
        std::vector<std::vector<std::wstring>> cells{{L"smth", L"else"}, {L"HBann", L"Sefu la bani"}};

        Sphere sphere(circle, std::make_unique<bool>(true), {"Commit: added tuple support", {22, 100}}, {circle, 22.});

        auto shapes = std::make_shared<std::vector<Shape *>>();
        shapes->push_back(new Circle(GUID_RND, "Circle1_SVG", "Circle1_URL", true));
        shapes->push_back(new RectangleEx(GUID_RND, std::move(sphere), cells));
        shapes->push_back(new Circle(GUID_RND, "Circle2_SVG", "Circle2_URL", std::vector{420., 69.}));

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
