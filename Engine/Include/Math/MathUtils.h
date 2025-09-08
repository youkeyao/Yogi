#pragma once

#include "Vector2.h"
#include "Vector3.h"
#include "Matrix4.h"

namespace Yogi
{

namespace MathUtils
{

inline float ToRadians(float degrees) { return degrees * 0.01745329251f; }
inline float ToDegrees(float radians) { return radians * 57.2957795131f; }
Matrix4      Perspective(float fov, float aspectRatio, float nearPlane, float farPlane);
Matrix4      Orthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane);
Matrix4      Inverse(const Matrix4& mat);

} // namespace MathUtils

} // namespace Yogi
