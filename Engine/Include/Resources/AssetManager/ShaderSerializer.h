#pragma once

#include "Resources/AssetManager/AssetSerializer.h"
#include "Renderer/RHI/IPipeline.h"

namespace Yogi
{

class YG_API ShaderSerializer : public AssetSerializer<ShaderDesc>
{
public:
    Handle<ShaderDesc>   Deserialize(const std::vector<uint8_t>& binary, const std::string& key) override;
    std::vector<uint8_t> Serialize(const Ref<ShaderDesc>& asset, const std::string& key) override;
};

} // namespace Yogi