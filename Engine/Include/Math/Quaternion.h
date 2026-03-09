#pragma once

#include "Math/Vector.h"
#include <glm/gtc/quaternion.hpp>

namespace Yogi
{

struct Quaternion : public glm::quat
{
    using glm::quat::quat;

    Quaternion(const glm::quat& other) : glm::quat(other) {}

    Quaternion(const Vector3& eulerAngles) :
        glm::quat(glm::radians(glm::vec3(eulerAngles.x, eulerAngles.y, eulerAngles.z)))
    {}

    Vector3 EulerAngles() const
    {
        glm::vec3 euler = glm::eulerAngles(*this);
        return Vector3(glm::degrees(euler.x), glm::degrees(euler.y), glm::degrees(euler.z));
    }

    static Quaternion AngleAxis(float angle, const glm::vec3& axis)
    {
        return Quaternion(glm::angleAxis(glm::radians(angle), axis));
    }
};

} // namespace Yogi
