#pragma once

#include "Resources/AssetManager/AssetSerializer.h"
#include "Renderer/RHI/ITexture.h"

namespace Yogi
{

class YG_API TextureSerializer : public AssetSerializer<ITexture>
{
public:
    Handle<ITexture>     Deserialize(const std::vector<uint8_t>& binary, const std::string& key) override;
    std::vector<uint8_t> Serialize(const Ref<ITexture>& asset, const std::string& key) override;
};

} // namespace Yogi
