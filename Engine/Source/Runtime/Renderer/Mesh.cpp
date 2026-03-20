#include "Renderer/Mesh.h"

#include <meshoptimizer.h>

namespace Yogi
{

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
    BuildMeshlets(vertices, indices);
}

void Mesh::BuildMeshlets(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
    size_t indexCount  = indices.size();
    size_t vertexCount = vertices.size();

    size_t maxMeshlets = meshopt_buildMeshletsBound(indexCount, MESHLET_MAX_VERTICES, MESHLET_MAX_TRIANGLES);
    std::vector<meshopt_Meshlet> tmpMeshlets(maxMeshlets);
    std::vector<unsigned int>    tmpVertices(maxMeshlets * MESHLET_MAX_VERTICES);
    std::vector<unsigned char>   tmpIndices(maxMeshlets * MESHLET_MAX_TRIANGLES * 3);

    size_t meshletCount = meshopt_buildMeshlets(tmpMeshlets.data(),
                                                tmpVertices.data(),
                                                tmpIndices.data(),
                                                indices.data(),
                                                indexCount,
                                                &vertices[0].Position.x,
                                                vertexCount,
                                                sizeof(Vertex),
                                                MESHLET_MAX_VERTICES,
                                                MESHLET_MAX_TRIANGLES,
                                                0.0f);

    m_vertices.resize(vertexCount);
    for (size_t i = 0; i < vertexCount; ++i)
    {
        m_vertices[i].vx = meshopt_quantizeHalf(vertices[i].Position.x);
        m_vertices[i].vy = meshopt_quantizeHalf(vertices[i].Position.y);
        m_vertices[i].vz = meshopt_quantizeHalf(vertices[i].Position.z);
        m_vertices[i].tx = meshopt_quantizeHalf(vertices[i].Texcoord.x);
        m_vertices[i].ty = meshopt_quantizeHalf(vertices[i].Texcoord.y);
        m_vertices[i].nx = (int8_t)meshopt_quantizeSnorm(vertices[i].Normal.x, 8);
        m_vertices[i].ny = (int8_t)meshopt_quantizeSnorm(vertices[i].Normal.y, 8);
        m_vertices[i].nz = (int8_t)meshopt_quantizeSnorm(vertices[i].Normal.z, 8);
    }

    m_meshlets.resize(meshletCount);
    for (size_t i = 0; i < meshletCount; ++i)
    {
        meshopt_optimizeMeshlet(&tmpVertices[tmpMeshlets[i].vertex_offset],
                                &tmpIndices[tmpMeshlets[i].triangle_offset],
                                tmpMeshlets[i].triangle_count,
                                tmpMeshlets[i].vertex_count);
        const auto& m               = tmpMeshlets[i];
        m_meshlets[i].VertexCount   = static_cast<uint8_t>(m.vertex_count);
        m_meshlets[i].TriangleCount = static_cast<uint8_t>(m.triangle_count);
        memcpy(m_meshlets[i].Vertices, &tmpVertices[m.vertex_offset], m.vertex_count * sizeof(unsigned int));
        memcpy(m_meshlets[i].Indices, &tmpIndices[m.triangle_offset], m.triangle_count * 3 * sizeof(unsigned char));

        meshopt_Bounds meshletBounds = meshopt_computeMeshletBounds(&tmpVertices[m.vertex_offset],
                                                        &tmpIndices[m.triangle_offset],
                                                        m.triangle_count,
                                                        &vertices[0].Position.x,
                                                        vertexCount,
                                                        sizeof(Vertex));
        m_meshlets[i].cone[0] = meshletBounds.cone_axis[0];
        m_meshlets[i].cone[1] = meshletBounds.cone_axis[1];
        m_meshlets[i].cone[2] = meshletBounds.cone_axis[2];
        m_meshlets[i].cone[3] = meshletBounds.cone_cutoff;
    }
}

} // namespace Yogi