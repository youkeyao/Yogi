#include "VulkanTexture.h"
#include "VulkanCommandBuffer.h"

namespace Yogi
{

VulkanTexture::VulkanTexture(const TextureDesc& desc) :
    m_width(desc.Width),
    m_height(desc.Height),
    m_format(desc.Format),
    m_usage(desc.Usage),
    m_numSamples(desc.NumSamples)
{
    CreateVkImage(desc.Width,
                  desc.Height,
                  (VkSampleCountFlagBits)desc.NumSamples,
                  YgTextureFormat2VkFormat(desc.Format),
                  VK_IMAGE_TILING_OPTIMAL,
                  VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                      (desc.Usage == ITexture::Usage::RenderTarget ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : 0) |
                      (desc.Usage == ITexture::Usage::DepthStencil ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : 0),
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    CreateVkImageView(m_image,
                      YgTextureFormat2VkFormat(desc.Format),
                      desc.Usage == ITexture::Usage::DepthStencil ? VK_IMAGE_ASPECT_DEPTH_BIT :
                                                                    VK_IMAGE_ASPECT_COLOR_BIT);
    CreateVkSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
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
    m_format(format),
    m_usage(usage)
{
    CreateVkImageView(image, YgTextureFormat2VkFormat(format), VK_IMAGE_ASPECT_COLOR_BIT);
}

VulkanTexture::~VulkanTexture()
{
    View<VulkanDeviceContext> deviceContext =
        static_cast<View<VulkanDeviceContext>>(Application::GetInstance().GetContext());
    VkDevice device = deviceContext->GetVkDevice();

    if (m_imageView != VK_NULL_HANDLE)
        vkDestroyImageView(device, m_imageView, nullptr);
    if (m_sampler != VK_NULL_HANDLE)
    {
        vkDestroyImage(device, m_image, nullptr);
        vkDestroySampler(device, m_sampler, nullptr);
    }
    if (m_imageMemory != VK_NULL_HANDLE)
        vkFreeMemory(device, m_imageMemory, nullptr);
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
    View<VulkanDeviceContext> deviceContext =
        static_cast<View<VulkanDeviceContext>>(Application::GetInstance().GetContext());
    VkPhysicalDevice physicalDevice = deviceContext->GetVkPhysicalDevice();
    VkDevice         device         = deviceContext->GetVkDevice();

    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width  = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = 1;
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

void VulkanTexture::CreateVkImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    View<VulkanDeviceContext> deviceContext =
        static_cast<View<VulkanDeviceContext>>(Application::GetInstance().GetContext());
    VkPhysicalDevice physicalDevice = deviceContext->GetVkPhysicalDevice();
    VkDevice         device         = deviceContext->GetVkDevice();

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                           = image;
    viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                          = format;
    viewInfo.subresourceRange.aspectMask     = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    VkResult result = vkCreateImageView(device, &viewInfo, nullptr, &m_imageView);
    YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to create texture image view!");
}

void VulkanTexture::CreateVkSampler(VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode)
{
    View<VulkanDeviceContext> deviceContext =
        static_cast<View<VulkanDeviceContext>>(Application::GetInstance().GetContext());
    VkDevice device = deviceContext->GetVkDevice();

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
    samplerInfo.maxLod                  = 0.0f;

    VkResult result = vkCreateSampler(device, &samplerInfo, nullptr, &m_sampler);
    YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to create texture sampler!");
}

} // namespace Yogi
