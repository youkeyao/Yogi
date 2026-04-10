#pragma once

#include "Resources/AssetManager/Serializer/AssetSerializer.h"
#include "Renderer/Material.h"

namespace Yogi
{

class YG_API MaterialSerializer : public AssetSerializer<Material>
{
public:
    Owner<Material>      Deserialize(const std::vector<uint8_t>& binary, const std::string& key) override;
    std::vector<uint8_t> Serialize(const WRef<Material>& asset, const std::string& key) override;
};

} // namespace Yogi
