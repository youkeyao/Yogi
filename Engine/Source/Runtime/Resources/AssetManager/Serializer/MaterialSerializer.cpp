#include "Resources/AssetManager/Serializer/MaterialSerializer.h"
#include "Resources/AssetManager/AssetManager.h"
#include "Resources/ResourceManager/ResourceManager.h"

#include <zpp_bits.h>

namespace Yogi
{

Handle<Material> MaterialSerializer::Deserialize(const std::vector<uint8_t>& binary, const std::string& key)
{
    zpp::bits::in inArchive(binary);

    std::string              pipelineKey;
    std::vector<std::string> textureKeys;
    std::vector<uint8_t>     data;
    auto                     result = inArchive(pipelineKey, data, textureKeys);
    if (failure(result))
    {
        YG_CORE_ERROR("Failed to deserialize material '{0}'!", key);
        return nullptr;
    }

    Handle<Material> material = Handle<Material>::Create();
    material->SetPipeline(AssetManager::GetAsset<IPipeline>(pipelineKey));
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

    std::vector<std::string> textureKeys;
    textureKeys.reserve(asset->GetTextures().size());
    for (auto& [index, texture] : asset->GetTextures())
    {
        textureKeys.push_back(AssetManager::GetAssetKey<ITexture>(texture));
    }
    auto result = outArchive(AssetManager::GetAssetKey<IPipeline>(asset->GetPipeline()), asset->GetData(), textureKeys);
    if (failure(result))
    {
        YG_CORE_ERROR("Failed to serialize material '{0}'!", key);
        return {};
    }
    return data;
}

} // namespace Yogi
