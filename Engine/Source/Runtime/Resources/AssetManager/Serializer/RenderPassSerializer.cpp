#include "Resources/AssetManager/Serializer/RenderPassSerializer.h"

#include <zpp_bits.h>

namespace Yogi
{

Handle<IRenderPass> RenderPassSerializer::Deserialize(const std::vector<uint8_t>& binary, const std::string& key)
{
    zpp::bits::in  inArchive(binary);
    RenderPassDesc renderPassDesc;
    auto           result = inArchive(renderPassDesc);
    if (failure(result))
    {
        YG_CORE_ERROR("Failed to serialize render pass '{0}'!", key);
        return nullptr;
    }
    return Handle<IRenderPass>::Create(renderPassDesc);
}

std::vector<uint8_t> RenderPassSerializer::Serialize(const Ref<IRenderPass>& asset, const std::string& key)
{
    std::vector<uint8_t> data;
    zpp::bits::out       outArchive(data);

    auto& renderPassDesc = asset->GetDesc();
    auto  result         = outArchive(renderPassDesc);
    if (failure(result))
    {
        YG_CORE_ERROR("Failed to serialize material '{0}'!", key);
        return {};
    }
    return data;
}

} // namespace Yogi