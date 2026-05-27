#include "VulkanTexture.h"
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
}

VulkanTexture::VulkanTexture(uint32_t         width,
                             uint32_t         height,
                             ITexture::Format format,
                             ITexture::Usage  usage,
                             VkImage          image) :
    m_image(image),
    m_ownsImage(false),
    m_width(width),
    m_height(height),
    m_mipLevels(1),
    m_format(format),
    m_usage(usage),
    m_numSamples(SampleCountFlagBits::Count1)
{}

VulkanTexture::~VulkanTexture()
{
    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());
    VkDevice             device  = context->GetVkDevice();

    if (m_ownsImage && m_image != VK_NULL_HANDLE)
    {
        vkDestroyImage(device, m_image, nullptr);
    }
    if (m_imageMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(device, m_imageMemory, nullptr);
    }
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

    QueueFamilyIndices indices           = FindQueueFamilies(physicalDevice, context->GetVkSurface());
    uint32_t           sharedFamilies[2] = { indices.graphicsFamily.value(), indices.transferFamily.value() };
    if (indices.graphicsFamily.value() != indices.transferFamily.value())
    {
        imageInfo.sharingMode           = VK_SHARING_MODE_CONCURRENT;
        imageInfo.queueFamilyIndexCount = 2;
        imageInfo.pQueueFamilyIndices   = sharedFamilies;
    }
    else
    {
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

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

} // namespace Yogi
