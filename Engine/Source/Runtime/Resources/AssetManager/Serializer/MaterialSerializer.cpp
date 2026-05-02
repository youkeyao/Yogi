#include "Resources/AssetManager/Serializer/MaterialSerializer.h"
#include "Resources/AssetManager/AssetManager.h"
#include "Resources/ResourceManager/ResourceManager.h"

#include <zpp_bits.h>

namespace Yogi
{

Owner<Material> MaterialSerializer::Deserialize(const std::vector<uint8_t>& binary, const std::string& key)
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

    Owner<Material> material = Owner<Material>::Create();
    for (auto& pipelineData : pipelineDatas)
    {
        bool hasMeshTaskShader = false;
        for (const auto& shaderKey : pipelineData.ShaderKeys)
        {
            if (shaderKey.find(".task") != std::string::npos || shaderKey.find(".mesh") != std::string::npos)
            {
                hasMeshTaskShader = true;
                break;
            }
        }

        if (hasMeshTaskShader)
        {
            bool hasBinding3 = false;
            bool hasBinding4 = false;
            for (auto& attr : pipelineData.ShaderResourceLayout)
            {
                if (attr.Binding == 3)
                {
                    hasBinding3 = true;
                    attr.Count  = 1;
                    attr.Type   = ShaderResourceType::StorageBuffer;
                    attr.Stage  = ShaderStage::Task | ShaderStage::Mesh;
                }
                if (attr.Binding == 4)
                {
                    hasBinding4 = true;
                    attr.Count  = 1;
                    attr.Type   = ShaderResourceType::StorageBuffer;
                    attr.Stage  = ShaderStage::Task;
                }
            }
            if (!hasBinding3)
            {
                pipelineData.ShaderResourceLayout.push_back(ShaderResourceAttribute{
                    3, 1, ShaderResourceType::StorageBuffer, ShaderStage::Task | ShaderStage::Mesh });
            }
            if (!hasBinding4)
            {
                pipelineData.ShaderResourceLayout.push_back(
                    ShaderResourceAttribute{ 4, 1, ShaderResourceType::StorageBuffer, ShaderStage::Task });
            }
        }

        PipelineDesc                  pipelineDesc;
        std::vector<WRef<ShaderDesc>> shaderRefs;
        shaderRefs.reserve(pipelineData.ShaderKeys.size());
        for (auto& shaderKey : pipelineData.ShaderKeys)
        {
            shaderRefs.push_back(AssetManager::AcquireAsset<ShaderDesc>(shaderKey));
            pipelineDesc.Shaders.push_back(shaderRefs.back().Get());
        }
        pipelineDesc.VertexLayout        = pipelineData.VertexLayout;
        WRef<IShaderResourceBinding> srb = ResourceManager::CreateResource<IShaderResourceBinding>(
            pipelineData.ShaderResourceLayout, pipelineData.PushConstantRanges);
        pipelineDesc.ShaderResourceBinding = srb.Get();
        WRef<IRenderPass> renderPass       = AssetManager::AcquireAsset<IRenderPass>(pipelineData.RenderPassKey);
        pipelineDesc.RenderPass            = renderPass.Get();
        pipelineDesc.SubPassIndex          = pipelineData.SubPassIndex;
        pipelineDesc.Topology              = pipelineData.Topology;

        auto passPipeline = ResourceManager::AcquireSharedResource<IPipeline>(pipelineDesc);
        material->AddPass(Material::MaterialPass{ pipelineData, passPipeline, {} });
    }
    auto materialPasses = material->GetPasses();
    int  textureIndex   = 0;
    for (int i = 0; i < materialPasses.size(); ++i)
    {
        materialPasses[i].Textures.resize(textureKeys.size());
        for (int j = 0; j < materialPasses[i].Textures.size(); ++j)
        {
            if (textureIndex < textureKeys.size() && !textureKeys[textureIndex].empty())
            {
                materialPasses[i].Textures[j] = AssetManager::AcquireAsset<ITexture>(textureKeys[textureIndex]);
            }
            ++textureIndex;
        }
        material->SetPass(i, materialPasses[i]);
    }

    return material;
}

std::vector<uint8_t> MaterialSerializer::Serialize(const WRef<Material>& asset, const std::string& key)
{
    std::vector<uint8_t> data;
    zpp::bits::out       outArchive(data);

    auto materialPasses = asset->GetPasses();

    std::vector<PipelineData>         pipelineDatas;
    std::vector<std::vector<uint8_t>> passData;
    std::vector<std::string>          textureKeys;
    for (auto& pass : materialPasses)
    {
        pipelineDatas.emplace_back(pass.PipelineInfo);
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
