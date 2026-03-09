#pragma once

#include "Math/Vector.h"
#include "Renderer/MeshData.h"

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
    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

    inline const std::vector<VertexData>&  GetVertices() const { return m_vertices; }
    inline const std::vector<MeshletData>& GetMeshlets() const { return m_meshlets; }

private:
    void BuildMeshlets(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

private:
    std::vector<VertexData>  m_vertices;
    std::vector<MeshletData> m_meshlets;
};

} // namespace Yogi
