#pragma once

#include "Renderer/Material.h"
#include "Renderer/MaterialSchema.h"
#include "Renderer/RHI/IBuffer.h"
#include "Renderer/RHI/ICommandBuffer.h"
#include "Renderer/ShaderData.h"
#include "Renderer/StagingArena.h"

namespace Yogi
{

class YG_API MaterialSlabUploader
{
public:
    using TextureResolver = MaterialSchema::TextureResolver;

    void BeginFrame();

    struct StagedRef
    {
        const MaterialSchema* Schema         = nullptr;
        uint32_t              SlabLocalIndex = 0;
    };
    StagedRef Stage(const Material* material, const TextureResolver& resolver);

    void Submit(ICommandBuffer* commandBuffer, StagingArena& stagingArena);

    uint64_t GetMaterialBufferAddr(const MaterialSchema* schema) const;
    uint64_t GetMaterialBufferAddr(const std::string& shaderKey) const;

    static constexpr uint32_t MAX_MATERIALS_PER_TYPE = 1024;

private:
    struct InternEntry
    {
        const MaterialSchema* Schema         = nullptr;
        uint32_t              SlabLocalIndex = 0;
    };
    std::unordered_map<const Material*, InternEntry> m_internMap;

    struct TypeBucket
    {
        WRef<MaterialSchema> Schema; // holds a reference to keep the schema alive
        WRef<IBuffer>        SlabBuffer;
        std::vector<uint8_t> Bytes;
        bool                 Overflowed = false;
    };

    TypeBucket* GetOrCreateBucket(const MaterialSchema* schema);

    std::unordered_map<const MaterialSchema*, TypeBucket> m_buckets;
};

} // namespace Yogi
