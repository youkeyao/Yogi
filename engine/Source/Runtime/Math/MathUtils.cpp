#include "Math/MathUtils.h"

namespace Yogi
{
namespace MathUtils
{

Matrix4 Perspective(float fov, float aspectRatio, float nearPlane, float farPlane)
{
    return glm::perspective(ToRadians(fov), aspectRatio, nearPlane, farPlane);
}

} // namespace MathUtils
} // namespace Yogi
