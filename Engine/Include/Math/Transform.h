#pragma once

#include "Math/Vector3.h"
#include "Math/Quaternion.h"
#include "Math/Matrix4.h"

namespace Yogi
{

struct Transform
{
    Vector3    Position;
    Quaternion Rotation;
    Vector3    Scale;

    Transform(const Vector3&    position = Vector3(0, 0, 0),
              const Quaternion& rotation = Quaternion(1, 0, 0, 0),
              const Vector3&    scale    = Vector3(1, 1, 1)) :
        Position(position),
        Rotation(rotation),
        Scale(scale)
    {}

    operator Matrix4() const
    {
        return Matrix4::Translation(Position) * Matrix4::Rotation(Rotation) * Matrix4::Scale(Scale);
    }

    Transform operator*(const Transform& other) const
    {
        Vector3    pos   = Position + Rotation * (Scale * other.Position);
        Quaternion rot   = Rotation * other.Rotation;
        Vector3    scale = Scale * other.Scale;
        return Transform(pos, rot, scale);
    }
};

} // namespace Yogi