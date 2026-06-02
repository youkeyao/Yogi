#pragma once

#include "Renderer/Mesh.h"
#include "Resources/ResourceManager/ResourceHash.h"

namespace Yogi
{

class YG_API MeshGPUUploadCache
{
public:
    struct Key
    {
        std::string AssetKey;
        uint32_t    SubmeshIndex      = 0;
        uint64_t    GeometrySignature = 0;

        bool operator==(const Key& rhs) const
        {
            return AssetKey == rhs.AssetKey && SubmeshIndex == rhs.SubmeshIndex &&
                GeometrySignature == rhs.GeometrySignature;
        }
    };

    struct KeyHash
    {
        size_t operator()(const Key& key) const
        {
            return static_cast<size_t>(HashArgs(key.AssetKey, key.SubmeshIndex, key.GeometrySignature));
        }
    };

public:
    static Key BuildKey(const WRef<Mesh>& mesh, const std::string& assetKey, uint32_t submeshIndex = 0);

    bool TryGet(const Key& key, uint32_t& outRecord) const;
    void Upsert(const Key& key, const uint32_t& record);
    void Clear();

    uint32_t VertexCount() const { return m_vertexCount; }
    uint32_t MeshletCount() const { return m_meshletCount; }
    uint32_t MeshletDataSize() const { return m_meshletDataSize; }
    uint32_t MeshCount() const { return m_meshCount; }

    void Advance(uint32_t vertexCount, uint32_t meshletCount, uint32_t meshletDataSize)
    {
        m_vertexCount += vertexCount;
        m_meshletCount += meshletCount;
        m_meshletDataSize += meshletDataSize;
        m_meshCount += 1;
    }

private:
    std::unordered_map<Key, uint32_t, KeyHash> m_records;

    uint32_t m_vertexCount     = 0;
    uint32_t m_meshletCount    = 0;
    uint32_t m_meshletDataSize = 0;
    uint32_t m_meshCount       = 0;
};

} // namespace Yogi
