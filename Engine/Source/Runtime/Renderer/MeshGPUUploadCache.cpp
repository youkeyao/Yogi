#include "Renderer/MeshGPUUploadCache.h"

namespace Yogi
{

MeshGPUUploadCache::Key MeshGPUUploadCache::BuildKey(const WRef<Mesh>&  mesh,
                                                     const std::string& assetKey,
                                                     uint32_t           submeshIndex)
{
    Key key{};
    if (!assetKey.empty())
    {
        key.AssetKey = assetKey;
    }
    else
    {
        key.AssetKey = std::to_string(reinterpret_cast<uintptr_t>(mesh.Get()));
    }

    key.SubmeshIndex      = submeshIndex;
    key.GeometrySignature = HashArgs(mesh->GetVertices().size(),
                                     mesh->GetMeshlets().size(),
                                     mesh->GetMeshletData().size(),
                                     sizeof(VertexData),
                                     sizeof(MeshletData));
    return key;
}

bool MeshGPUUploadCache::TryGet(const Key& key, uint32_t& outRecord) const
{
    auto it = m_records.find(key);
    if (it == m_records.end())
        return false;

    outRecord = it->second;
    return true;
}

void MeshGPUUploadCache::Upsert(const Key& key, const uint32_t& record)
{
    m_records[key] = record;
}

void MeshGPUUploadCache::Clear()
{
    m_records.clear();
}

} // namespace Yogi
