#include "Resources/AssetManager/Serializer/PipelineSerializer.h"
#include "Resources/AssetManager/AssetManager.h"
#include "Resources/ResourceManager/ResourceManager.h"

#include <zpp_bits.h>

namespace Yogi
{

Handle<IPipeline> PipelineSerializer::Deserialize(const std::vector<uint8_t>& binary, const std::string& key)
{
    zpp::bits::in inArchive(binary);

    PipelineDesc                         pipelineDesc;
    std::vector<std::string>             shaderKeys;
    std::vector<ShaderResourceAttribute> shaderResourceLayout;
    std::string                          renderPassKey;

    auto result = inArchive(shaderKeys,
                            pipelineDesc.VertexLayout,
                            shaderResourceLayout,
                            renderPassKey,
                            pipelineDesc.SubPassIndex,
                            pipelineDesc.Topology);
    if (failure(result))
    {
        YG_CORE_ERROR("Failed to serialize pipeline '{0}'!", key);
        return nullptr;
    }

    for (auto& shaderKey : shaderKeys)
    {
        pipelineDesc.Shaders.push_back(AssetManager::GetAsset<ShaderDesc>(shaderKey));
    }
    auto shaderResourceBinding = ResourceManager::GetResource<IShaderResourceBinding>(shaderResourceLayout);
    pipelineDesc.ShaderResourceBinding = shaderResourceBinding;
    pipelineDesc.RenderPass = AssetManager::GetAsset<IRenderPass>(renderPassKey);

    return Handle<IPipeline>::Create(pipelineDesc);
}

std::vector<uint8_t> PipelineSerializer::Serialize(const Ref<IPipeline>& asset, const std::string& key)
{
    std::vector<uint8_t> data;
    zpp::bits::out       outArchive(data);

    auto&                    pipelineDesc = asset->GetDesc();
    std::vector<std::string> shaderKeys;
    shaderKeys.reserve(pipelineDesc.Shaders.size());
    for (auto& shader : pipelineDesc.Shaders)
    {
        shaderKeys.push_back(AssetManager::GetAssetKey<ShaderDesc>(shader));
    }

    auto result = outArchive(shaderKeys,
                             pipelineDesc.VertexLayout,
                             pipelineDesc.ShaderResourceBinding->GetLayout(),
                             AssetManager::GetAssetKey<IRenderPass>(pipelineDesc.RenderPass),
                             pipelineDesc.SubPassIndex,
                             pipelineDesc.Topology);
    if (failure(result))
    {
        YG_CORE_ERROR("Failed to serialize pipeline '{0}'!", key);
        return {};
    }
    return data;
}

} // namespace Yogi