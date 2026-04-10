#pragma once

#include "Resources/AssetManager/Serializer/AssetSerializer.h"
#include "Renderer/RHI/IRenderPass.h"

namespace Yogi
{

class YG_API RenderPassSerializer : public AssetSerializer<IRenderPass>
{
public:
    Owner<IRenderPass>   Deserialize(const std::vector<uint8_t>& binary, const std::string& key) override;
    std::vector<uint8_t> Serialize(const WRef<IRenderPass>& asset, const std::string& key) override;
};

} // namespace Yogi
