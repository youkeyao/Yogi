#pragma once

#include "Resources/AssetManager/Serializer/AssetSerializer.h"
#include "Renderer/RHI/IPipeline.h"

namespace Yogi
{

class YG_API PipelineSerializer : public AssetSerializer<IPipeline>
{
public:
    Handle<IPipeline>    Deserialize(const std::vector<uint8_t>& binary, const std::string& key) override;
    std::vector<uint8_t> Serialize(const Ref<IPipeline>& asset, const std::string& key) override;
};

} // namespace Yogi
