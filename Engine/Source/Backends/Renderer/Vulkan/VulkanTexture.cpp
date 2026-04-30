#include "VulkanTexture.h"
#include "VulkanCommandBuffer.h"
#include "VulkanBuffer.h"
#include "VulkanUtils.h"

#include "Math/MathUtils.h"

#include <volk.h>

namespace Yogi
{

Owner<ITexture> ITexture::Create(const TextureDesc& desc)
{
    return Owner<VulkanTexture>::Create(desc);
}

VulkanTexture::VulkanTexture(const TextureDesc& desc) :
    m_width(desc.Width),
    m_height(desc.Height),
    m_mipLevels(MathUtils::Max(1u, desc.MipLevels)),
    m_format(desc.Format),
    m_usage(desc.Usage),
    m_numSamples(desc.NumSamples)
{
    CreateVkImage(desc.Width,
                  desc.Height,
                  (VkSampleCountFlagBits)desc.NumSamples,
                  YgTextureFormat2VkFormat(desc.Format),
                  VK_IMAGE_TILING_OPTIMAL,
                  VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                      (desc.Usage == ITexture::Usage::Storage ? VK_IMAGE_USAGE_STORAGE_BIT : 0) |
                      (desc.Usage == ITexture::Usage::RenderTarget ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : 0) |
                      (desc.Usage == ITexture::Usage::DepthStencil ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : 0),
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    const VkImageAspectFlags aspectFlags =
        desc.Usage == ITexture::Usage::DepthStencil ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    for (uint32_t mip = 0; mip < m_mipLevels; ++mip)
    {
        m_imageViews.push_back(CreateVkImageView(m_image, YgTextureFormat2VkFormat(desc.Format), aspectFlags, mip, 1));
    }
    CreateVkSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, desc.Reduction);
}

VulkanTexture::VulkanTexture(uint32_t         width,
                             uint32_t         height,
                             ITexture::Format format,
                             ITexture::Usage  usage,
                             VkImage          image) :
    m_image(image),
    m_sampler(VK_NULL_HANDLE),
    m_width(width),
    m_height(height),
    m_mipLevels(1),
    m_format(format),
    m_usage(usage)
{
    m_imageViews.push_back(CreateVkImageView(image, YgTextureFormat2VkFormat(format), VK_IMAGE_ASPECT_COLOR_BIT, 0, 1));
}

VulkanTexture::~VulkanTexture()
{
    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());
    VkDevice             device  = context->GetVkDevice();

    for (auto& imageView : m_imageViews)
    {
        if (imageView != VK_NULL_HANDLE)
            vkDestroyImageView(device, imageView, nullptr);
    }
    m_imageViews.clear();
    if (m_sampler != VK_NULL_HANDLE)
    {
        vkDestroyImage(device, m_image, nullptr);
        vkDestroySampler(device, m_sampler, nullptr);
    }
    if (m_imageMemory != VK_NULL_HANDLE)
        vkFreeMemory(device, m_imageMemory, nullptr);
}

void VulkanTexture::SetData(void* data, uint32_t size)
{
    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());
    VkDevice             device  = context->GetVkDevice();

    VulkanCommandBuffer commandBuffer({ CommandBufferUsage::OneTimeSubmit, SubmitQueue::Transfer });
    commandBuffer.Begin();

    commandBuffer.Barrier(BarrierDesc{
        .Texture      = this,
        .BeforeState  = ResourceState::Common,
        .AfterState   = ResourceState::CopyDestination,
        .BaseMipLevel = 0,
        .LevelCount   = 1,
    });

    VkBufferImageCopy region{};
    region.bufferOffset                    = 0;
    region.bufferRowLength                 = 0;
    region.bufferImageHeight               = 0;
    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;
    region.imageOffset                     = { 0, 0, 0 };
    region.imageExtent                     = { m_width, m_height, 1 };
    VulkanBuffer stagingBuffer(BufferDesc{ size, BufferUsage::Staging, BufferAccess::Dynamic });
    stagingBuffer.UpdateData(data, size);
    vkCmdCopyBufferToImage(commandBuffer.GetVkCommandBuffer(),
                           stagingBuffer.GetVkBuffer(),
                           m_image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &region);

    commandBuffer.Barrier(BarrierDesc{
        .Texture      = this,
        .BeforeState  = ResourceState::CopyDestination,
        .AfterState   = ResourceState::FragmentShaderResource,
        .BaseMipLevel = 0,
        .LevelCount   = 1,
    });
    commandBuffer.End();
    commandBuffer.Submit();
}

// ----------------------------------------------------------------------------------------------

void VulkanTexture::CreateVkImage(uint32_t              width,
                                  uint32_t              height,
                                  VkSampleCountFlagBits numSamples,
                                  VkFormat              format,
                                  VkImageTiling         tiling,
                                  VkImageUsageFlags     usage,
                                  VkMemoryPropertyFlags properties)
{
    VulkanDeviceContext* context        = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());
    VkPhysicalDevice     physicalDevice = context->GetVkPhysicalDevice();
    VkDevice             device         = context->GetVkDevice();

    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width  = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = m_mipLevels;
    imageInfo.arrayLayers   = 1;
    imageInfo.format        = format;
    imageInfo.tiling        = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage         = usage;
    imageInfo.samples       = numSamples;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateImage(device, &imageInfo, nullptr, &m_image);
    YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to create image!");

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, m_image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, physicalDevice, properties);

    result = vkAllocateMemory(device, &allocInfo, nullptr, &m_imageMemory);
    YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to allocate image memory!");

    vkBindImageMemory(device, m_image, m_imageMemory, 0);
}

VkImageView VulkanTexture::CreateVkImageView(VkImage            image,
                                             VkFormat           format,
                                             VkImageAspectFlags aspectFlags,
                                             uint32_t           baseMipLevel,
                                             uint32_t           levelCount)
{
    VulkanDeviceContext* context        = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());
    VkPhysicalDevice     physicalDevice = context->GetVkPhysicalDevice();
    VkDevice             device         = context->GetVkDevice();

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                           = image;
    viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                          = format;
    viewInfo.subresourceRange.aspectMask     = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel   = baseMipLevel;
    viewInfo.subresourceRange.levelCount     = levelCount;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    VkImageView imageView = VK_NULL_HANDLE;
    VkResult    result    = vkCreateImageView(device, &viewInfo, nullptr, &imageView);
    YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to create texture image view!");
    return imageView;
}

void VulkanTexture::CreateVkSampler(VkFilter                       magFilter,
                                    VkFilter                       minFilter,
                                    VkSamplerAddressMode           addressMode,
                                    ITexture::SamplerReductionMode reduction)
{
    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());
    VkDevice             device  = context->GetVkDevice();

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter               = magFilter;
    samplerInfo.minFilter               = minFilter;
    samplerInfo.addressModeU            = addressMode;
    samplerInfo.addressModeV            = addressMode;
    samplerInfo.addressModeW            = addressMode;
    samplerInfo.anisotropyEnable        = VK_FALSE;
    samplerInfo.maxAnisotropy           = 1.0f;
    samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable           = VK_FALSE;
    samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.mipLodBias              = 0.0f;
    samplerInfo.minLod                  = 0.0f;
    samplerInfo.maxLod                  = static_cast<float>(m_mipLevels - 1);

    VkSamplerReductionModeCreateInfo reductionInfo{};
    if (reduction != ITexture::SamplerReductionMode::None)
    {
        reductionInfo.sType         = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO;
        reductionInfo.reductionMode = (reduction == ITexture::SamplerReductionMode::Max) ?
            VK_SAMPLER_REDUCTION_MODE_MAX :
            VK_SAMPLER_REDUCTION_MODE_MIN;
        samplerInfo.pNext           = &reductionInfo;
    }

    VkResult result = vkCreateSampler(device, &samplerInfo, nullptr, &m_sampler);
    YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to create texture sampler!");
}

} // namespace Yogi
