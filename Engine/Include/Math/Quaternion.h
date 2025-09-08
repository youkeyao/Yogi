#pragma once

#include <glm/gtc/quaternion.hpp>

namespace Yogi
{

struct Quaternion : public glm::quat
{
    using glm::quat::quat;

    Quaternion(const glm::quat& other) : glm::quat(other) {}
};

} // namespace Yogi
