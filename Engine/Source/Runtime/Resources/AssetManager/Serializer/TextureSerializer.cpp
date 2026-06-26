#include "Resources/AssetManager/Serializer/TextureSerializer.h"
#include "Renderer/RHI/ITextureView.h"

#include <stb_image.h>
#include <zpp_bits.h>

namespace Yogi
{

Owner<ITexture> TextureSerializer::Deserialize(const std::vector<uint8_t>& binary, const std::string& key)
{
    std::string ext = std::filesystem::path(key).extension().string();

    if (ext != ".rt")
    {
        int           width, height, channels;
        constexpr int kReqChannels = 4;
        stbi_uc* data = stbi_load_from_memory(binary.data(), binary.size(), &width, &height, &channels, kReqChannels);
        if (!data)
        {
            YG_CORE_ERROR("Failed to load texture '{0}'!", key);
            return nullptr;
        }

        TextureDesc desc{};
        desc.Width      = (uint32_t)width;
        desc.Height     = (uint32_t)height;
        desc.Format     = ITexture::Format::R8G8B8A8_UNORM;
        desc.UsageFlags = TextureUsageFlags::Sampled | TextureUsageFlags::TransferDst;

        Owner<ITexture> texture = Owner<ITexture>::Create(desc);

        // Transient view for the upload — texture survives, view dies at scope exit.
        Owner<ITextureView> uploadView = ITextureView::Create(WRef<ITexture>::Create(texture));
        uploadView->SetData(data, width * height * kReqChannels);

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
        Owner<ITexture> texture = Owner<ITexture>::Create(TextureDesc{
            (uint32_t)width,
            (uint32_t)height,
            1,
            format,
        });
        return texture;
    }
}

std::vector<uint8_t> TextureSerializer::Serialize(const WRef<ITexture>& asset, const std::string& key)
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
