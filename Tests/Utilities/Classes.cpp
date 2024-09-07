#include "pch.h"
#include "Classes.h"

hbann::IStreamable *Shape::FindDerivedStreamable(hbann::StreamReader &aStreamReader)
{
    Type type{};
    aStreamReader.ReadAll(type);

    switch (type)
    {
        using enum Shape::Type;

    case CIRCLE:
        return new Circle;
    case RECTANGLE:
        return new RectangleEx;

    case NONE:
    default:
        return nullptr;
    }
}
