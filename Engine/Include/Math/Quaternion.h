#pragma once

#include <glm/gtc/quaternion.hpp>

namespace Yogi
{

struct Quaternion : public glm::quat
{
    using glm::quat::quat;

    Quaternion(const glm::quat& other) : glm::quat(other) {}

    static Quaternion AngleAxis(float angle, const glm::vec3& axis)
    {
        return Quaternion(glm::angleAxis(glm::radians(angle), axis));
    }
};

} // namespace Yogi
