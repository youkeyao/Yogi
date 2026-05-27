#include "Resources/AssetManager/Serializer/MaterialSerializer.h"
#include "Resources/AssetManager/AssetManager.h"
#include "Resources/ResourceManager/ResourceManager.h"

#include <zpp_bits.h>

namespace Yogi
{

Owner<Material> MaterialSerializer::Deserialize(const std::vector<uint8_t>& binary, const std::string& key)
{
    zpp::bits::in inArchive(binary);

    std::vector<std::string> textureKeys;
    auto                     result = inArchive(textureKeys);
    if (failure(result))
    {
        YG_CORE_ERROR("Failed to deserialize material '{0}'!", key);
        return nullptr;
    }

    Owner<Material> material = Owner<Material>::Create();
    material->Textures.resize(textureKeys.size());
    for (size_t i = 0; i < textureKeys.size(); ++i)
    {
        if (!textureKeys[i].empty())
        {
            material->Textures[i] = AssetManager::AcquireAsset<ITexture>(textureKeys[i]);
        }
    }
    return material;
}

std::vector<uint8_t> MaterialSerializer::Serialize(const WRef<Material>& asset, const std::string& key)
{
    std::vector<uint8_t> data;
    zpp::bits::out       outArchive(data);

    std::vector<std::string> textureKeys;
    textureKeys.reserve(asset->Textures.size());
    for (auto& texture : asset->Textures)
    {
        textureKeys.emplace_back(AssetManager::GetAssetKey<ITexture>(texture));
    }

    auto result = outArchive(textureKeys);
    if (failure(result))
    {
        YG_CORE_ERROR("Failed to serialize material '{0}'!", key);
        return {};
    }
    return data;
}

} // namespace Yogi
