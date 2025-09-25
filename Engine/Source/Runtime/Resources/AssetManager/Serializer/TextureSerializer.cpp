#include "Resources/AssetManager/Serializer/TextureSerializer.h"

#include <stb_image.h>

namespace Yogi
{

Handle<ITexture> TextureSerializer::Deserialize(const std::vector<uint8_t>& binary, const std::string& key)
{
    int      width, height, channels;
    stbi_uc* data = stbi_load_from_memory(binary.data(), binary.size(), &width, &height, &channels, 0);
    if (!data)
    {
        YG_CORE_ERROR("Failed to load texture '{0}'!", key);
        return nullptr;
    }

    Handle<ITexture> texture = Handle<ITexture>::Create(TextureDesc{
        (uint32_t)width,
        (uint32_t)height,
        1,
        channels == 4 ? ITexture::Format::R8G8B8A8_UNORM : ITexture::Format::R8G8B8_UNORM,
        ITexture::Usage::Texture2D,
        SampleCountFlagBits::Count1,
    });

    texture->SetData(data, width * height * channels);

    stbi_image_free(data);

    return texture;
}

std::vector<uint8_t> TextureSerializer::Serialize(const Ref<ITexture>& asset, const std::string& key) { return {}; }

} // namespace Yogi
