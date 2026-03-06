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

struct Meshlet
{
    static const uint32_t MAX_TRIANGLES = 42;
    static const uint32_t MAX_VERTICES  = 64;
    uint32_t              Vertices[MAX_VERTICES];
    uint8_t               Indices[MAX_TRIANGLES * 3];
    uint8_t               TriangleCount;
    uint8_t               VertexCount;
};

class YG_API Mesh
{
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

    inline const std::vector<Vertex>&   GetVertices() const { return m_vertices; }
    inline const std::vector<uint32_t>& GetIndices() const { return m_indices; }
    inline const std::vector<Meshlet>&  GetMeshlets() const { return m_meshlets; }

private:
    void BuildMeshlets();

private:
    std::vector<Vertex>   m_vertices;
    std::vector<uint32_t> m_indices;
    std::vector<Meshlet>  m_meshlets;
};

} // namespace Yogi
