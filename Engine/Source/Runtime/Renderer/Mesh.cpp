#include "Renderer/Mesh.h"

#include <meshoptimizer.h>

namespace Yogi
{

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) :
    m_vertices(vertices),
    m_indices(indices)
{
    BuildMeshlets();
}

void Mesh::BuildMeshlets()
{
    if (m_indices.empty() || m_vertices.empty())
    {
        m_meshlets.clear();
        return;
    }

    size_t maxMeshlets = meshopt_buildMeshletsBound(m_indices.size(), Meshlet::MAX_VERTICES, Meshlet::MAX_TRIANGLES);

    std::vector<meshopt_Meshlet> tmpMeshlets(maxMeshlets);
    std::vector<unsigned int>    tmpVertices(maxMeshlets * Meshlet::MAX_VERTICES);
    std::vector<unsigned char>   tmpIndices(maxMeshlets * Meshlet::MAX_TRIANGLES * 3);

    size_t meshletCount = meshopt_buildMeshlets(tmpMeshlets.data(),
                                                tmpVertices.data(),
                                                tmpIndices.data(),
                                                m_indices.data(),
                                                m_indices.size(),
                                                &m_vertices[0].Position.x,
                                                m_vertices.size(),
                                                sizeof(Vertex),
                                                Meshlet::MAX_VERTICES,
                                                Meshlet::MAX_TRIANGLES,
                                                0);

    m_meshlets.resize(meshletCount);
    for (size_t i = 0; i < meshletCount; ++i)
    {
        const meshopt_Meshlet& src = tmpMeshlets[i];
        Meshlet&               dst = m_meshlets[i];

        dst.VertexCount   = static_cast<uint8_t>(src.vertex_count);
        dst.TriangleCount = static_cast<uint8_t>(src.triangle_count);

        std::memcpy(dst.Vertices, &tmpVertices[src.vertex_offset], src.vertex_count * sizeof(unsigned int));
        std::memcpy(dst.Indices, &tmpIndices[src.triangle_offset], src.triangle_count * 3 * sizeof(unsigned char));
    }
}

} // namespace Yogi