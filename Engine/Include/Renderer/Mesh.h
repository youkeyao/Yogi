#pragma once

#include "Math/Vector3.h"
#include "Math/Vector2.h"

namespace Yogi
{

struct Vertex
{
    Vector3 Position;
    Vector3 Normal;
    Vector2 Texcoord;
};

class YG_API Mesh
{
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) :
        m_vertices(vertices),
        m_indices(indices)
    {}

    inline const std::vector<Vertex>&   GetVertices() const { return m_vertices; }
    inline const std::vector<uint32_t>& GetIndices() const { return m_indices; }

private:
    std::vector<Vertex>   m_vertices;
    std::vector<uint32_t> m_indices;
};

} // namespace Yogi
