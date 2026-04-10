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

private:
    std::unordered_map<Key, uint32_t, KeyHash> m_records;
};

} // namespace Yogi
