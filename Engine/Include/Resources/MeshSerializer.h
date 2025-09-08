#pragma once

#include "Resources/AssetSerializer.h"
#include "Renderer/Mesh.h"

namespace Yogi
{

class YG_API MeshSerializer : public AssetSerializer<Mesh>
{
public:
    Scope<Mesh> Deserialize(const std::vector<uint8_t>& binary, const std::string& key) override;
    std::vector<uint8_t> Serialize(const Scope<Mesh>& asset, const std::string& key) override;
};
    
} // namespace Yogi
