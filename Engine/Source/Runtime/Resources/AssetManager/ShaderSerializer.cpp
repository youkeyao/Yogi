#include "Resources/AssetManager/ShaderSerializer.h"

namespace Yogi
{

Handle<ShaderDesc> ShaderSerializer::Deserialize(const std::vector<uint8_t>& binary, const std::string& key)
{
    std::filesystem::path path(key);
    if (path.extension() == ".vert")
        return Handle<ShaderDesc>::Create(ShaderStage::Vertex, binary);
    else if (path.extension() == ".frag")
        return Handle<ShaderDesc>::Create(ShaderStage::Fragment, binary);
    return nullptr;
}

std::vector<uint8_t> ShaderSerializer::Serialize(const Ref<ShaderDesc>& asset, const std::string& key) { return {}; }

} // namespace Yogi
