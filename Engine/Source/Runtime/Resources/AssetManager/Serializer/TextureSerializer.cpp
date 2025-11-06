#include "Resources/AssetManager/Serializer/TextureSerializer.h"

#include <stb_image.h>
#include <zpp_bits.h>

namespace Yogi
{

Handle<ITexture> TextureSerializer::Deserialize(const std::vector<uint8_t>& binary, const std::string& key)
{
    std::string ext = std::filesystem::path(key).extension().string();

    if (ext != ".rt")
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
    else
    {
        zpp::bits::in    inArchive(binary);
        int              width, height;
        ITexture::Format format;
        auto             result = inArchive(width, height, format);
        if (failure(result))
        {
            YG_CORE_ERROR("Failed to serialize render texture '{0}'!", key);
            return nullptr;
        }
        Handle<ITexture> texture = Handle<ITexture>::Create(TextureDesc{
            (uint32_t)width,
            (uint32_t)height,
            1,
            format,
            ITexture::Usage::RenderTarget,
            SampleCountFlagBits::Count1,
        });
        return texture;
    }
}

std::vector<uint8_t> TextureSerializer::Serialize(const Ref<ITexture>& asset, const std::string& key)
{
    std::string ext = std::filesystem::path(key).extension().string();

    if (ext != ".rt")
        return {};

    std::vector<uint8_t> data;
    zpp::bits::out       outArchive(data);

    auto result = outArchive(asset->GetWidth(), asset->GetHeight(), asset->GetFormat());
    if (failure(result))
    {
        YG_CORE_ERROR("Failed to serialize render texture '{0}'!", key);
        return {};
    }
    return data;
}

} // namespace Yogi
