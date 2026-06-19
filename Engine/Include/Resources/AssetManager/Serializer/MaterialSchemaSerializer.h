#pragma once

#include "Resources/AssetManager/Serializer/AssetSerializer.h"
#include "Renderer/MaterialSchema.h"

namespace Yogi
{

class YG_API MaterialSchemaSerializer : public AssetSerializer<MaterialSchema>
{
public:
    Owner<MaterialSchema> Deserialize(const std::vector<uint8_t>& binary, const std::string& key) override;
    std::vector<uint8_t>  Serialize(const WRef<MaterialSchema>& asset, const std::string& key) override;
};

} // namespace Yogi
