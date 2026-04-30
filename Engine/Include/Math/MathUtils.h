#pragma once

#include "Math/Vector.h"
#include "Math/Matrix.h"

namespace Yogi
{

namespace MathUtils
{

inline float ToRadians(float degrees)
{
    return degrees * 0.01745329251f;
}
inline float ToDegrees(float radians)
{
    return radians * 57.2957795131f;
}

template <typename T>
constexpr const T& Min(const T& a, const T& b)
{
    return (b < a) ? b : a;
}

template <typename T>
constexpr const T& Max(const T& a, const T& b)
{
    return (a < b) ? b : a;
}

template <typename T>
constexpr const T& Clamp(const T& value, const T& minValue, const T& maxValue)
{
    return (value < minValue) ? minValue : ((maxValue < value) ? maxValue : value);
}

Matrix4  Perspective(float fov, float aspectRatio, float nearPlane, float farPlane);
Matrix4  Orthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane);
Matrix4  Inverse(const Matrix4& mat);
uint32_t PreviousPow2(uint32_t v);

} // namespace MathUtils

} // namespace Yogi
