#pragma once

#include "Math/Vector3.h"
#include "Math/Quaternion.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Yogi
{

struct Matrix4 : public glm::mat4
{
    using glm::mat4::mat4;

    Matrix4(const glm::mat4& mat) : glm::mat4(mat) {}

    static Matrix4 Translation(const Vector3& translation)
    {
        return glm::translate(glm::mat4(1.0f), glm::vec3(translation.x, translation.y, translation.z));
    }

    static Matrix4 Rotation(float angleX, float angleY, float angleZ)
    {
        glm::mat4 rx = glm::rotate(glm::mat4(1.0f), glm::radians(angleX), Vector3(1, 0, 0));
        glm::mat4 ry = glm::rotate(glm::mat4(1.0f), glm::radians(angleY), Vector3(0, 1, 0));
        glm::mat4 rz = glm::rotate(glm::mat4(1.0f), glm::radians(angleZ), Vector3(0, 0, 1));
        return rz * ry * rx;
    }

    static Matrix4 Rotation(float angle, const Vector3& axis)
    {
        return glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(axis.x, axis.y, axis.z));
    }

    static Matrix4 Rotation(const Quaternion& quaternion)
    {
        return glm::mat4_cast(quaternion);
    }

    static Matrix4 Scale(const Vector3& scale)
    {
        return glm::scale(glm::mat4(1.0f), glm::vec3(scale.x, scale.y, scale.z));
    }

    static Matrix4 Identity() { return glm::mat4(1.0f); }
};

} // namespace Yogi
