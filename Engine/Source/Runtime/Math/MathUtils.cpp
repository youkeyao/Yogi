#include "Math/MathUtils.h"

namespace Yogi
{
namespace MathUtils
{

Matrix4 Perspective(float fov, float aspectRatio, float nearPlane, float farPlane)
{
    return glm::perspective(ToRadians(fov), aspectRatio, nearPlane, farPlane);
}

Matrix4 Orthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane)
{
    return glm::ortho(left, right, bottom, top, nearPlane, farPlane);
}

Matrix4 Inverse(const Matrix4& mat)
{
    return glm::inverse(mat);
}

uint32_t PreviousPow2(uint32_t v)
{
    uint32_t r = 1;
    while (r * 2 <= v)
        r *= 2;
    return r;
}

} // namespace MathUtils
} // namespace Yogi
