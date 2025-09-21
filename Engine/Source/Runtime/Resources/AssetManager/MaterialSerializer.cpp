#include "Resources/AssetManager/MaterialSerializer.h"
#include "Resources/AssetManager/AssetManager.h"

#include <zpp_bits.h>

namespace Yogi
{

struct MaterialDesc
{
    std::string              PipelineName;
    std::vector<std::string> TextureKeys;
    std::vector<uint8_t>     Data;
};

Handle<Material> MaterialSerializer::Deserialize(const std::vector<uint8_t>& binary, const std::string& key)
{
    zpp::bits::in            inArchive(binary);
    std::string              pipelineName;
    std::vector<std::string> textureKeys;
    std::vector<uint8_t>     data;
    if (auto result = inArchive(pipelineName, textureKeys, data); failure(result))
    {
        YG_CORE_ERROR("Failed to deserialize material '{0}'!", key);
        return nullptr;
    }

    Ref<IPipeline>   pipeline = AssetManager::GetAsset<IPipeline>(pipelineName);
    Handle<Material> material = Material::Create(pipeline);
    for (int32_t i = 0; i < textureKeys.size(); ++i)
    {
        if (!textureKeys[i].empty())
        {
            Ref<ITexture> texture = AssetManager::GetAsset<ITexture>(textureKeys[i]);
            material->SetTexture(i, texture);
        }
    }
    material->GetData() = std::move(data);

    return material;
}

std::vector<uint8_t> MaterialSerializer::Serialize(const Ref<Material>& asset, const std::string& key)
{
    // std::vector<uint8_t> data;
    // zpp::bits::out outArchive(data);

    // auto& pipelineDesc = asset->GetPipeline()->GetDesc();
    // for (auto& shaders : pipelineDesc.Shaders)
    // {
    //     outArchive(AssetRegistry::GetKey<ShaderDesc>(shaders));
    // }
    // return data;
    return {};
}

} // namespace Yogi
