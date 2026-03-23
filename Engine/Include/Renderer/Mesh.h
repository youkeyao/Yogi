#pragma once

#include "Math/Vector.h"
#include "Renderer/ShaderData.h"

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
    inline const std::vector<uint32_t>&    GetMeshletData() const { return m_meshletData; }
    inline const Vector3&                  GetCenter() const { return m_center; }
    inline float                           GetBoundingRadius() const { return m_boundingRadius; }

private:
    void BuildMeshlets(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

private:
    std::vector<VertexData>  m_vertices;
    std::vector<MeshletData> m_meshlets;
    std::vector<uint32_t>    m_meshletData;
    Vector3                  m_center;
    float                    m_boundingRadius = 1.0f;
};

} // namespace Yogi
