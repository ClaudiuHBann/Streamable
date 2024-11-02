#pragma once

struct v1 : public hbann::IStreamable
{
    STREAMABLE_DEFINE(v1, a)

  public:
    int a{};
};

struct v2 : public hbann::IStreamable
{
    STREAMABLE_DEFINE(v2, a, b)

  public:
    int a{};
    int b{};
};

struct vv1 : public hbann::IStreamable
{
    STREAMABLE_DEFINE(vv1, a, b)

  public:
    int a{};
    std::string b{};
};

struct vv2 : public hbann::IStreamable
{
    STREAMABLE_DEFINE(vv2, a)

  public:
    int a{};
};

struct vvv1 : public hbann::IStreamable
{
    STREAMABLE_DEFINE(vvv1, a)

  public:
    std::vector<int> a{};
};

struct vvv2 : public hbann::IStreamable
{
    STREAMABLE_DEFINE(vvv2)
};

TEST_CASE("Compatibility::Backwards", "[Compatibility][Backwards]")
{
    SECTION("Forwards/Backwards 1")
    {
        v1 v1;
        v2 v2;

        v1.a = 420;

        v2.a = 420;
        v2.b = 69;

        v1.Deserialize(v2.Serialize());
        REQUIRE(v1.a == v2.a);

        v1.a = 420;

        v2.a = 420;
        v2.b = 69;

        v2.Deserialize(v1.Serialize());
        REQUIRE(v2.a == v1.a);
    }

    SECTION("Forwards/Backwards 2")
    {
        vv1 vv1;
        vv2 vv2;

        vv1.a = 420;
        vv1.b = "69";

        vv2.a = 420;

        vv1.Deserialize(vv2.Serialize());
        REQUIRE(vv1.a == vv2.a);

        vv1.a = 420;
        vv1.b = "69";

        vv2.a = 420;

        vv2.Deserialize(vv1.Serialize());
        REQUIRE(vv2.a == vv1.a);
    }

    SECTION("Forwards/Backwards 3")
    {
        const std::vector v{420, 69};

        vvv1 vvv1;
        vvv2 vvv2;

        vvv1.a = v;

        vvv1.Deserialize(vvv2.Serialize());
        // still 2 because we dont overwrite the property
        REQUIRE(vvv1.a.size() == 2);

        vvv1.a = v;

        vvv2.Deserialize(vvv1.Serialize());
        REQUIRE(v == vvv1.a);
    }
}
