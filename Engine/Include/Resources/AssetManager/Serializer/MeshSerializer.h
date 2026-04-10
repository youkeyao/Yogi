#pragma once

#include "Resources/AssetManager/Serializer/AssetSerializer.h"
#include "Renderer/Mesh.h"

namespace Yogi
{

class YG_API MeshSerializer : public AssetSerializer<Mesh>
{
public:
    Owner<Mesh>          Deserialize(const std::vector<uint8_t>& binary, const std::string& key) override;
    std::vector<uint8_t> Serialize(const WRef<Mesh>& asset, const std::string& key) override;
};

} // namespace Yogi
