#pragma once

#include <glm/glm.hpp>

namespace Yogi
{

struct Vector4 : public glm::vec4
{
    using glm::vec4::vec4;

    Vector4(const glm::vec4& other) : glm::vec4(other) {}
};

} // namespace Yogi
