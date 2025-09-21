#pragma once

#include "Resources/AssetManager/AssetSerializer.h"
#include "Renderer/Material.h"

namespace Yogi
{

class YG_API MaterialSerializer : public AssetSerializer<Material>
{
public:
    Handle<Material>     Deserialize(const std::vector<uint8_t>& binary, const std::string& key) override;
    std::vector<uint8_t> Serialize(const Ref<Material>& asset, const std::string& key) override;
};

} // namespace Yogi
