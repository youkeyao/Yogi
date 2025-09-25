#pragma once

#include "Resources/AssetManager/Serializer/AssetSerializer.h"
#include "Renderer/RHI/IRenderPass.h"

namespace Yogi
{

class YG_API RenderPassSerializer : public AssetSerializer<IRenderPass>
{
public:
    Handle<IRenderPass>  Deserialize(const std::vector<uint8_t>& binary, const std::string& key) override;
    std::vector<uint8_t> Serialize(const Ref<IRenderPass>& asset, const std::string& key) override;
};

} // namespace Yogi
