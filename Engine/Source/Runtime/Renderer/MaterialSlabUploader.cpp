#include "Renderer/MaterialSlabUploader.h"
#include "Renderer/StagingArena.h"
#include "Renderer/ShaderData.h"
#include "Resources/ResourceManager/ResourceManager.h"
#include "Resources/AssetManager/AssetManager.h"

namespace Yogi
{

void MaterialSlabUploader::BeginFrame()
{
    // Remove buckets whose schema WRef has expired (schema was destroyed).
    std::vector<const MaterialSchema*> expiredKeys;
    for (auto& [schema, bucket] : m_buckets)
    {
        if (bucket.Schema.Expired())
            expiredKeys.push_back(schema);
        else
            bucket.Overflowed = false;
    }
    for (const auto* key : expiredKeys)
        m_buckets.erase(key);
}

MaterialSlabUploader::TypeBucket* MaterialSlabUploader::GetOrCreateBucket(const MaterialSchema* schema)
{
    if (!schema || schema->Stride() == 0)
        return nullptr;

    auto it = m_buckets.find(schema);
    if (it != m_buckets.end())
        return &it->second;

    TypeBucket bucket{};
    // Acquire a WRef from AssetManager to keep the schema alive while the bucket exists.
    // The schema is already in AssetManager (acquired by MaterialSerializer or Material),
    // so this just creates a new WRef that shares the same control block.
    std::string key = AssetManager::GetAssetKey(schema);
    if (!key.empty())
    {
        WRef<MaterialSchema> schemaRef = AssetManager::AcquireAsset<MaterialSchema>(key);
        bucket.Schema                  = schemaRef;
    }

    bucket.SlabBuffer = ResourceManager::CreateResource<IBuffer>(
        BufferDesc{ static_cast<uint64_t>(schema->Stride()) * MAX_MATERIALS_PER_TYPE, BufferUsage::Storage });
    auto [inserted, _] = m_buckets.emplace(schema, std::move(bucket));
    return &inserted->second;
}

MaterialSlabUploader::StagedRef MaterialSlabUploader::Stage(const Material* material, const TextureResolver& resolver)
{
    StagedRef out;
    if (!material)
        return out;

    const MaterialSchema* schema = material->Schema.Get();
    if (!schema || schema->Stride() == 0)
        return out;

    TypeBucket* bucket = GetOrCreateBucket(schema);
    if (!bucket)
        return out;

    if (bucket->Overflowed)
        return out;

    out.Schema = schema;

    const uint32_t stride = schema->Stride();

    // Check global InternMap.
    auto it = m_internMap.find(material);
    if (it != m_internMap.end())
    {
        // Found previous entry.
        if (it->second.Schema == schema)
        {
            // Same type: reuse index, overwrite data in-place.
            const uint32_t idx      = it->second.SlabLocalIndex;
            const size_t   required = (static_cast<size_t>(idx) + 1) * stride;
            if (bucket->Bytes.size() < required)
                bucket->Bytes.resize(required);
            schema->Pack(*material, bucket->Bytes.data() + static_cast<size_t>(idx) * stride, resolver);
            out.SlabLocalIndex = idx;
            return out;
        }
        else
        {
            // Type mismatch: pointer reused for a different material type.
            // Remove old entry; treat as new material.
            m_internMap.erase(it);
            // fall through to new material logic.
        }
    }

    // New material.
    const uint32_t idx = static_cast<uint32_t>(bucket->Bytes.size() / stride);
    if (idx >= MAX_MATERIALS_PER_TYPE)
    {
        YG_CORE_ERROR("MaterialSlabUploader: schema='{0}' hit MAX_MATERIALS_PER_TYPE={1}",
                      AssetManager::GetAssetKey(schema),
                      MAX_MATERIALS_PER_TYPE);
        bucket->Overflowed = true;
        return out;
    }

    const size_t offset = static_cast<size_t>(idx) * stride;
    if (bucket->Bytes.size() < offset + stride)
        bucket->Bytes.resize(offset + stride);
    schema->Pack(*material, bucket->Bytes.data() + offset, resolver);
    m_internMap[material] = { schema, idx };

    out.SlabLocalIndex = idx;
    return out;
}

void MaterialSlabUploader::Submit(ICommandBuffer* commandBuffer, StagingArena& stagingArena)
{
    for (auto& [schema, bucket] : m_buckets)
    {
        IBuffer* buf = bucket.SlabBuffer.Get();
        if (buf && !bucket.Bytes.empty())
        {
            stagingArena.Stage(commandBuffer,
                               buf,
                               /*dstOffset*/ 0,
                               bucket.Bytes.data(),
                               bucket.Bytes.size());
        }
    }
}

uint64_t MaterialSlabUploader::GetMaterialBufferAddr(const MaterialSchema* schema) const
{
    if (!schema)
        return 0;
    auto it = m_buckets.find(schema);
    if (it != m_buckets.end())
    {
        IBuffer* buf = it->second.SlabBuffer.Get();
        return buf ? buf->GetDeviceAddress() : 0;
    }
    return 0;
}

uint64_t MaterialSlabUploader::GetMaterialBufferAddr(const std::string& shaderKey) const
{
    WRef<MaterialSchema> schema = AssetManager::AcquireAsset<MaterialSchema>(shaderKey);
    if (!schema)
        return 0;
    return GetMaterialBufferAddr(schema.Get());
}

} // namespace Yogi
