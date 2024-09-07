#pragma once

typedef struct _guid
{
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
} guid;

inline constexpr guid GUID_RND = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

class Shape : public virtual hbann::IStreamable
{
    STREAMABLE_DEFINE(Shape, mType, mID)

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

    Type GetType() const
    {
        return mType;
    }

  private:
    Type mType = Type::NONE;
    guid mID{};

    static hbann::IStreamable *FindDerivedStreamable(hbann::StreamReader &aStreamReader);
};

class Circle : public Shape
{
    STREAMABLE_DEFINE_BASE(Shape)
    STREAMABLE_DEFINE(Circle, mSVG, mURL, mVariant)

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
    STREAMABLE_DEFINE_BASE(Circle)
    STREAMABLE_DEFINE(Sphere, mReflexion, mTuple, mPair)

  public:
    Sphere() = default;
    Sphere(const Circle &aCircle, std::unique_ptr<bool> &&aReflexion, std::tuple<std::string, std::list<int>> &&aTuple,
           std::pair<Circle, double> &&aPair)
        : Circle(aCircle), mReflexion(std::move(aReflexion)), mTuple(std::move(aTuple)), mPair(std::move(aPair))
    {
    }

    bool operator==(const Sphere &aSphere) const
    {
        return *(Circle *)this == *(Circle *)&aSphere && *mReflexion == *aSphere.mReflexion &&
               mTuple == aSphere.mTuple && mPair == aSphere.mPair;
    }

  private:
    std::unique_ptr<bool> mReflexion{};
    std::tuple<std::string, std::list<int>> mTuple{};
    std::pair<Circle, double> mPair{};
};

class RectangleEx : public Shape
{
    STREAMABLE_DEFINE_BASE(Shape)
    STREAMABLE_DEFINE(RectangleEx, mCenter, mMap, mCells)

  public:
    RectangleEx() = default;
    RectangleEx(const guid &aID, Sphere &&aCenter, const std::vector<std::vector<std::wstring>> &aCells)
        : Shape(Type::RECTANGLE, aID), mCenter(std::move(aCenter)), mCells(aCells)
    {
    }

    bool operator==(const RectangleEx &aRectangle) const
    {
        return *(Shape *)this == *(Shape *)&aRectangle && mCenter == aRectangle.mCenter && mCells == aRectangle.mCells;
    }

  private:
    Sphere mCenter{};
    std::map<int, double> mMap{};
    std::vector<std::vector<std::wstring>> mCells{};
};

class Diamond : public Circle, public RectangleEx
{
    STREAMABLE_DEFINE_BASE(Circle, RectangleEx)
    STREAMABLE_DEFINE(Diamond)

  public:
    Diamond() = default;
    Diamond(const guid &aIDC, const std::optional<std::string> &aSVG, const std::filesystem::path &aURL,
            std::variant<std::vector<double>, bool> &&aVariant, const guid &aIDR, Sphere &&aCenter,
            const std::vector<std::vector<std::wstring>> &aCells)
        : Circle(aIDC, aSVG, aURL, std::move(aVariant)), RectangleEx(aIDR, std::move(aCenter), aCells)
    {
    }

    bool operator==(const Diamond &aDiamond) const
    {
        return *(Circle *)this == *(Circle *)&aDiamond && *(RectangleEx *)this == *(RectangleEx *)&aDiamond;
    }
};

class Context : public hbann::IStreamable
{
    STREAMABLE_DEFINE(Context, mShapes)

  public:
    Context() = default;
    Context(std::shared_ptr<std::vector<Shape *>> &&aShapes) : mShapes(std::move(aShapes))
    {
    }

    ~Context()
    {
        for (auto &shape : *mShapes)
        {
            delete shape;
        }
    }

    bool operator==(const Context &aContext) const
    {
        if (mShapes->size() != aContext.mShapes->size())
        {
            return false;
        }

        for (size_t i = 0; i < mShapes->size(); i++)
        {
            if ((*mShapes)[i]->GetType() != (*aContext.mShapes)[i]->GetType())
            {
                return false;
            }

            switch ((*mShapes)[i]->GetType())
            {
                using enum Shape::Type;

            case CIRCLE: {
                if (*(Circle *)(*mShapes)[i] != *(Circle *)(*aContext.mShapes)[i])
                {
                    return false;
                }

                break;
            }

            case RECTANGLE: {
                if (*(RectangleEx *)(*mShapes)[i] != *(RectangleEx *)(*aContext.mShapes)[i])
                {
                    return false;
                }

                break;
            }

            case NONE:
            default:
                return false;
            }
        }

        return true;
    }

  private:
    std::shared_ptr<std::vector<Shape *>> mShapes{};
};
