#pragma once

#include <glm/glm.hpp>

namespace Yogi
{

struct Vector3 : public glm::vec3
{
    using glm::vec3::vec3;

    static Vector3 Up() { return Vector3(0, 1, 0); }
    static Vector3 Right() { return Vector3(1, 0, 0); }
    static Vector3 Backward() { return Vector3(0, 0, 1); }
};

} // namespace Yogi
