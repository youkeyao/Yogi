#include "VulkanTextureView.h"
#include "VulkanTexture.h"
#include "VulkanCommandBuffer.h"
#include "VulkanBuffer.h"
#include "VulkanUtils.h"

#include "Math/MathUtils.h"

#include <volk.h>

namespace Yogi
{

Owner<ITextureView> ITextureView::Create(const WRef<ITexture>& texture, const TextureViewDesc& desc)
{
    return Owner<VulkanTextureView>::Create(texture, desc);
}

VulkanTextureView::VulkanTextureView(const WRef<ITexture>& texture, const TextureViewDesc& desc) : m_texture(texture)
{
    const ITexture* tex = m_texture.Get();
    YG_CORE_ASSERT(tex, "VulkanTextureView: texture is null or already destroyed");

    m_baseMip    = desc.BaseMipLevel;
    m_mipCount   = (desc.MipLevelCount == 0) ? tex->GetMipLevels() - desc.BaseMipLevel : desc.MipLevelCount;
    m_baseLayer  = desc.BaseArrayLayer;
    m_layerCount = desc.ArrayLayerCount;
    m_format     = (desc.Format == ITexture::Format::NONE) ? tex->GetFormat() : desc.Format;
    YG_CORE_ASSERT(m_format == tex->GetFormat(), "VulkanTextureView: format reinterpret not yet supported");

    YG_CORE_ASSERT(m_baseMip + m_mipCount <= tex->GetMipLevels(), "VulkanTextureView: mip range out of bounds");

    m_aspectMask =
        (tex->GetUsage() == ITexture::Usage::DepthStencil) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    const VulkanTexture* vkTex = static_cast<const VulkanTexture*>(tex);

    VkImageViewCreateInfo info{};
    info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image                           = vkTex->GetVkImage();
    info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    info.format                          = YgTextureFormat2VkFormat(m_format);
    info.subresourceRange.aspectMask     = m_aspectMask;
    info.subresourceRange.baseMipLevel   = m_baseMip;
    info.subresourceRange.levelCount     = m_mipCount;
    info.subresourceRange.baseArrayLayer = m_baseLayer;
    info.subresourceRange.layerCount     = m_layerCount;

    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());
    VkResult             result  = vkCreateImageView(context->GetVkDevice(), &info, nullptr, &m_imageView);
    YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to create texture view!");
}

VulkanTextureView::~VulkanTextureView()
{
    if (m_imageView != VK_NULL_HANDLE)
    {
        VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());
        vkDestroyImageView(context->GetVkDevice(), m_imageView, nullptr);
        m_imageView = VK_NULL_HANDLE;
    }
}

VkImageSubresourceRange VulkanTextureView::GetVkSubresourceRange() const
{
    VkImageSubresourceRange range{};
    range.aspectMask     = m_aspectMask;
    range.baseMipLevel   = m_baseMip;
    range.levelCount     = m_mipCount;
    range.baseArrayLayer = m_baseLayer;
    range.layerCount     = m_layerCount;
    return range;
}

void VulkanTextureView::SetData(void* data, uint32_t size)
{
    const ITexture* tex = m_texture.Get();
    YG_CORE_ASSERT(tex, "VulkanTextureView::SetData: texture has been destroyed");

    const VulkanTexture* vkTex = static_cast<const VulkanTexture*>(tex);

    VulkanCommandBuffer commandBuffer({ CommandBufferUsage::OneTimeSubmit, SubmitQueue::Transfer });
    commandBuffer.Begin();

    commandBuffer.Barrier(BarrierDesc{
        .TextureView = this,
        .BeforeState = ResourceState::Common,
        .AfterState  = ResourceState::CopyDestination,
    });

    VkBufferImageCopy region{};
    region.bufferOffset                    = 0;
    region.bufferRowLength                 = 0;
    region.bufferImageHeight               = 0;
    region.imageSubresource.aspectMask     = m_aspectMask;
    region.imageSubresource.mipLevel       = m_baseMip;
    region.imageSubresource.baseArrayLayer = m_baseLayer;
    region.imageSubresource.layerCount     = m_layerCount;
    region.imageOffset                     = { 0, 0, 0 };
    region.imageExtent                     = { MathUtils::Max(1u, tex->GetWidth() >> m_baseMip),
                                               MathUtils::Max(1u, tex->GetHeight() >> m_baseMip),
                                               1 };

    VulkanBuffer stagingBuffer(BufferDesc{ size, BufferUsage::Staging });
    void*        mapped = stagingBuffer.GetMappedPtr();
    YG_CORE_ASSERT(mapped, "VulkanTextureView::SetData: staging buffer was not mapped");
    memcpy(mapped, data, size);
    vkCmdCopyBufferToImage(commandBuffer.GetVkCommandBuffer(),
                           stagingBuffer.GetVkBuffer(),
                           vkTex->GetVkImage(),
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &region);

    commandBuffer.Barrier(BarrierDesc{
        .TextureView = this,
        .BeforeState = ResourceState::CopyDestination,
        .AfterState  = ResourceState::FragmentShaderResource,
    });
    commandBuffer.End();
    commandBuffer.Submit();
}

} // namespace Yogi
