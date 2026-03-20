#pragma once

#include <glm/glm.hpp>

namespace Yogi
{

template <int N, typename T = float>
struct Vector : public glm::vec<N, T, glm::defaultp>
{
    using glm::vec<N, T, glm::defaultp>::vec;

    Vector(const glm::vec<N, T, glm::defaultp>& other) : glm::vec<N, T, glm::defaultp>(other) {}

    Vector Cross(const Vector& other) const { return glm::cross(*this, other); }

    Vector Normalized() const { return glm::normalize(*this); }
};

struct Vector2 : public Vector<2, float>
{
    using Vector<2, float>::Vector;
};

struct Vector3 : public Vector<3, float>
{
    using Vector<3, float>::Vector;

    static Vector3 Up() { return Vector3(0, 1, 0); }
    static Vector3 Right() { return Vector3(1, 0, 0); }
    static Vector3 Backward() { return Vector3(0, 0, 1); }
};

struct Vector4 : public Vector<4, float>
{
    using Vector<4, float>::Vector;
};

} // namespace Yogi
