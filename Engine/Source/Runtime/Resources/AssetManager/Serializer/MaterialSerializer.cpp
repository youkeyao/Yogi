#include "Resources/AssetManager/Serializer/MaterialSerializer.h"
#include "Resources/AssetManager/AssetManager.h"
#include "Resources/ResourceManager/ResourceManager.h"

#include <zpp_bits.h>

namespace Yogi
{

Handle<Material> MaterialSerializer::Deserialize(const std::vector<uint8_t>& binary, const std::string& key)
{
    zpp::bits::in inArchive(binary);

    std::vector<std::string> shaderKeys;
    std::string              renderPassKey;
    PipelineDesc             pipelineDesc;
    std::vector<std::string> textureKeys;
    std::vector<uint8_t>     data;
    auto                     result =
        inArchive(shaderKeys, renderPassKey, pipelineDesc.SubPassIndex, pipelineDesc.Topology, data, textureKeys);
    if (failure(result))
    {
        YG_CORE_ERROR("Failed to deserialize material '{0}'!", key);
        return nullptr;
    }

    for (auto& shaderKey : shaderKeys)
    {
        pipelineDesc.Shaders.push_back(AssetManager::GetAsset<ShaderDesc>(shaderKey));
    }
    pipelineDesc.RenderPass = AssetManager::GetAsset<IRenderPass>(renderPassKey);

    Ref<IPipeline>   pipeline = ResourceManager::GetResource<IPipeline>(pipelineDesc);
    Handle<Material> material = Material::Create();
    material->SetPipeline(pipeline);
    for (int32_t i = 0; i < textureKeys.size(); ++i)
    {
        if (!textureKeys[i].empty())
        {
            material->SetTexture(i, AssetManager::GetAsset<ITexture>(textureKeys[i]));
        }
    }
    material->SetData(data);

    return material;
}

std::vector<uint8_t> MaterialSerializer::Serialize(const Ref<Material>& asset, const std::string& key)
{
    std::vector<uint8_t> data;
    zpp::bits::out       outArchive(data);

    auto&                    pipelineDesc = asset->GetPipeline()->GetDesc();
    std::vector<std::string> shaderKeys;
    shaderKeys.reserve(pipelineDesc.Shaders.size());
    for (auto& shader : pipelineDesc.Shaders)
    {
        shaderKeys.push_back(AssetManager::GetAssetKey<ShaderDesc>(shader));
    }
    std::vector<std::string> textureKeys;
    textureKeys.reserve(asset->GetTextures().size());
    for (auto& [index, texture] : asset->GetTextures())
    {
        textureKeys.push_back(AssetManager::GetAssetKey<ITexture>(texture));
    }
    auto result = outArchive(shaderKeys,
                             AssetManager::GetAssetKey<IRenderPass>(pipelineDesc.RenderPass),
                             pipelineDesc.SubPassIndex,
                             static_cast<uint8_t>(pipelineDesc.Topology),
                             asset->GetData(),
                             textureKeys);
    if (failure(result))
    {
        YG_CORE_ERROR("Failed to serialize material '{0}'!", key);
        return {};
    }
    return data;
}

} // namespace Yogi
