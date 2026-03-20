#include "Resources/AssetManager/Serializer/MaterialSerializer.h"
#include "Resources/AssetManager/AssetManager.h"
#include "Resources/ResourceManager/ResourceManager.h"

#include <zpp_bits.h>

namespace Yogi
{

struct PipelineData
{
    std::vector<std::string>             ShaderKeys;
    std::vector<VertexAttribute>         VertexLayout;
    std::vector<ShaderResourceAttribute> ShaderResourceLayout;
    std::vector<PushConstantRange>       PushConstantRanges;
    std::string                          RenderPassKey;
    int                                  SubPassIndex;
    PrimitiveTopology                    Topology;
};

Handle<Material> MaterialSerializer::Deserialize(const std::vector<uint8_t>& binary, const std::string& key)
{
    zpp::bits::in inArchive(binary);

    std::vector<PipelineData>         pipelineDatas;
    std::vector<std::vector<uint8_t>> passData;
    std::vector<std::string>          textureKeys;
    auto                              result = inArchive(pipelineDatas, passData, textureKeys);
    if (failure(result))
    {
        YG_CORE_ERROR("Failed to deserialize material '{0}'!", key);
        return nullptr;
    }

    Handle<Material> material = Handle<Material>::Create();
    for (auto& pipelineData : pipelineDatas)
    {
        PipelineDesc pipelineDesc;
        for (auto& shaderKey : pipelineData.ShaderKeys)
        {
            pipelineDesc.Shaders.push_back(AssetManager::GetAsset<ShaderDesc>(shaderKey));
        }
        pipelineDesc.VertexLayout = pipelineData.VertexLayout;
        pipelineDesc.ShaderResourceBinding =
            ResourceManager::GetResource<IShaderResourceBinding>(pipelineData.ShaderResourceLayout,
                                                                 pipelineData.PushConstantRanges);
        pipelineDesc.RenderPass   = AssetManager::GetAsset<IRenderPass>(pipelineData.RenderPassKey);
        pipelineDesc.SubPassIndex = pipelineData.SubPassIndex;
        pipelineDesc.Topology     = pipelineData.Topology;

        auto passPipeline = ResourceManager::GetResource<IPipeline>(pipelineDesc);
        material->AddPass(Material::MaterialPass{ passPipeline, {} });
    }
    auto materialPasses = material->GetPasses();
    int textureIndex = 0;
    for (int i = 0; i < materialPasses.size(); ++i)
    {
        materialPasses[i].Textures.resize(textureKeys.size());
        for (int j = 0; j < materialPasses[i].Textures.size(); ++j)
        {
            if (textureIndex < textureKeys.size() && !textureKeys[textureIndex].empty())
            {
                materialPasses[i].Textures[j] = AssetManager::GetAsset<ITexture>(textureKeys[textureIndex]);
            }
            ++textureIndex;
        }
        material->SetPass(i, materialPasses[i]);
    }

    return material;
}

std::vector<uint8_t> MaterialSerializer::Serialize(const Ref<Material>& asset, const std::string& key)
{
    std::vector<uint8_t> data;
    zpp::bits::out       outArchive(data);

    auto materialPasses = asset->GetPasses();

    std::vector<PipelineData>         pipelineDatas;
    std::vector<std::vector<uint8_t>> passData;
    std::vector<std::string>          textureKeys;
    for (auto& pass : materialPasses)
    {
        auto&        desc = pass.Pipeline->GetDesc();
        PipelineData pipelineData;
        for (auto& shader : desc.Shaders)
        {
            pipelineData.ShaderKeys.push_back(AssetManager::GetAssetKey<ShaderDesc>(shader));
        }
        pipelineData.VertexLayout         = desc.VertexLayout;
        pipelineData.ShaderResourceLayout = desc.ShaderResourceBinding->GetLayout();
        pipelineData.PushConstantRanges   = desc.ShaderResourceBinding->GetPushConstantRanges();
        pipelineData.RenderPassKey        = AssetManager::GetAssetKey<IRenderPass>(desc.RenderPass);
        pipelineData.SubPassIndex         = desc.SubPassIndex;
        pipelineData.Topology             = desc.Topology;
        pipelineDatas.emplace_back(pipelineData);
        passData.emplace_back();
        for (auto& texture : pass.Textures)
        {
            textureKeys.emplace_back(AssetManager::GetAssetKey<ITexture>(texture));
        }
    }
    auto result = outArchive(pipelineDatas, passData, textureKeys);
    if (failure(result))
    {
        YG_CORE_ERROR("Failed to serialize material '{0}'!", key);
        return {};
    }
    return data;
}

} // namespace Yogi
