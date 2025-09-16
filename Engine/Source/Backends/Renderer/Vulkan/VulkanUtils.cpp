#include "VulkanUtils.h"

#include <volk.h>

namespace Yogi
{

bool CheckValidationLayerSupport(const std::vector<const char*>& validationLayers)
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    for (const char* layerName : validationLayers)
    {
        bool layerFound = false;
        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }
        if (!layerFound)
        {
            return false;
        }
    }
    return true;
}
QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    QueueFamilyIndices indices;
    uint32_t           queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }
        if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT && !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT))
        {
            indices.computeFamily = i;
        }
        if (!indices.computeFamily.has_value() && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT))
        {
            indices.computeFamily = i;
        }
        if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT && !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT))
        {
            indices.transferFamily = i;
        }
        if (!indices.transferFamily.has_value() && (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT))
        {
            indices.transferFamily = i;
        }
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport)
        {
            indices.presentFamily = i;
        }
        if (indices.isComplete())
        {
            break;
        }
        ++i;
    }
    return indices;
}
bool CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& deviceExtensions)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }
    return requiredExtensions.empty();
}
bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<const char*>& deviceExtensions)
{
    QueueFamilyIndices indices             = FindQueueFamilies(device, surface);
    bool               extensionsSupported = CheckDeviceExtensionSupport(device, deviceExtensions);
    return indices.isComplete() && extensionsSupported;
}

SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }
    return details;
}
VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}
VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, const Ref<Window>& window)
{
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }
    else
    {
        int width  = window->GetWidth();
        int height = window->GetHeight();

        VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
        actualExtent.width =
            std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height =
            std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return actualExtent;
    }
}

uint32_t FindMemoryType(uint32_t typeFilter, VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    YG_CORE_ERROR("Vulkan: Failed to find suitable memory type!");
    return 0;
}

VkFormat YgTextureFormat2VkFormat(ITexture::Format format)
{
    switch (format)
    {
        case ITexture::Format::R8G8B8A8_UNORM:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case ITexture::Format::R8G8B8A8_SRGB:
            return VK_FORMAT_R8G8B8A8_SRGB;
        case ITexture::Format::B8G8R8A8_UNORM:
            return VK_FORMAT_B8G8R8A8_UNORM;
        case ITexture::Format::B8G8R8A8_SRGB:
            return VK_FORMAT_B8G8R8A8_SRGB;
        case ITexture::Format::D32_FLOAT:
            return VK_FORMAT_D32_SFLOAT;
        case ITexture::Format::D24_UNORM_S8_UINT:
            return VK_FORMAT_D24_UNORM_S8_UINT;
        default:
            YG_CORE_ERROR("Vulkan: Unsupported Yg texture format!");
            return VK_FORMAT_UNDEFINED;
    }
}

ITexture::Format VkFormat2YgTextureFormat(VkFormat format)
{
    switch (format)
    {
        case VK_FORMAT_R8G8B8A8_UNORM:
            return ITexture::Format::R8G8B8A8_UNORM;
        case VK_FORMAT_R8G8B8A8_SRGB:
            return ITexture::Format::R8G8B8A8_SRGB;
        case VK_FORMAT_B8G8R8A8_UNORM:
            return ITexture::Format::B8G8R8A8_UNORM;
        case VK_FORMAT_B8G8R8A8_SRGB:
            return ITexture::Format::B8G8R8A8_SRGB;
        case VK_FORMAT_D32_SFLOAT:
            return ITexture::Format::D32_FLOAT;
        case VK_FORMAT_D24_UNORM_S8_UINT:
            return ITexture::Format::D24_UNORM_S8_UINT;
        default:
            YG_CORE_ERROR("Vulkan: Unsupported Vk texture format!");
            return ITexture::Format::NONE;
    }
}

VkFormat YgShaderElementType2VkFormat(ShaderElementType type)
{
    switch (type)
    {
        case ShaderElementType::Float:
            return VK_FORMAT_R32_SFLOAT;
        case ShaderElementType::Float2:
            return VK_FORMAT_R32G32_SFLOAT;
        case ShaderElementType::Float3:
            return VK_FORMAT_R32G32B32_SFLOAT;
        case ShaderElementType::Float4:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        case ShaderElementType::Int:
            return VK_FORMAT_R32_SINT;
        case ShaderElementType::Int2:
            return VK_FORMAT_R32G32_SINT;
        case ShaderElementType::Int3:
            return VK_FORMAT_R32G32B32_SINT;
        case ShaderElementType::Int4:
            return VK_FORMAT_R32G32B32A32_SINT;
        default:
            YG_CORE_ERROR("Vulkan: Unsupported shader element type!");
            return VK_FORMAT_UNDEFINED;
    }
}

VkImageLayout AttachmentUsage2VkImageLayout(AttachmentUsage usage)
{
    switch (usage)
    {
        case AttachmentUsage::Color:
            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case AttachmentUsage::DepthStencil:
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        case AttachmentUsage::ShaderRead:
            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case AttachmentUsage::Present:
            return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        default:
            YG_CORE_ERROR("Vulkan: Unsupported attachment usage!");
            return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

VkAccessFlags AccessMaskFromImageLayout(VkImageLayout Layout, bool IsDstMask)
{
    VkAccessFlags AccessMask = 0;
    switch (Layout)
    {
        // does not support device access. This layout must only be used as the initialLayout member
        // of VkImageCreateInfo or VkAttachmentDescription, or as the oldLayout in an image transition.
        // When transitioning out of this layout, the contents of the memory are not guaranteed to be preserved
        case VK_IMAGE_LAYOUT_UNDEFINED:
            break;

        // supports all types of device access
        case VK_IMAGE_LAYOUT_GENERAL:
            // VK_IMAGE_LAYOUT_GENERAL must be used for image load/store operations
            AccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            break;

        // must only be used as a color or resolve attachment in a VkFramebuffer
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            AccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        // must only be used as a depth/stencil attachment in a VkFramebuffer
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            AccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        // must only be used as a read-only depth/stencil attachment in a VkFramebuffer and/or as a read-only image in a shader
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
            AccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            break;

        // must only be used as a read-only image in a shader (which can be read as a sampled image,
        // combined image/sampler and/or input attachment)
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            AccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
            break;

        //  must only be used as a source image of a transfer command
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            AccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        // must only be used as a destination image of a transfer command
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            AccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        // does not support device access. This layout must only be used as the initialLayout member
        // of VkImageCreateInfo or VkAttachmentDescription, or as the oldLayout in an image transition.
        // When transitioning out of this layout, the contents of the memory are preserved.
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            if (!IsDstMask)
            {
                AccessMask = VK_ACCESS_HOST_WRITE_BIT;
            }
            break;

        case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
            AccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
            AccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            break;

        // When transitioning the image to VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR or VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        // there is no need to delay subsequent processing, or perform any visibility operations (as vkQueuePresentKHR
        // performs automatic visibility operations). To achieve this, the dstAccessMask member of the VkImageMemoryBarrier
        // should be set to 0, and the dstStageMask parameter should be set to VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT.
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            AccessMask = 0;
            break;

        case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
            AccessMask = VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
            break;

        case VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT:
            AccessMask = VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT;
            break;

        default:
            YG_CORE_ERROR("Vulkan: Unexpected image layout to access mask mapping");
            break;
    }

    return AccessMask;
}

VkPipelineStageFlags PipelineStageFromImageLayout(VkImageLayout Layout, bool IsDstStage)
{
    VkPipelineStageFlags StageMask = 0;
    switch (Layout)
    {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            if (!IsDstStage)
            {
                StageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            }
            break;

        case VK_IMAGE_LAYOUT_GENERAL:
            StageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            StageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            StageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
            StageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT |
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            StageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
                VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            StageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            StageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;

        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            if (!IsDstStage)
            {
                StageMask = VK_PIPELINE_STAGE_HOST_BIT;
            }
            break;

        case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
            StageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT |
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
            StageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT |
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            break;

        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            StageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR:
            StageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            break;

        case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
            StageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
            break;

        case VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT:
            StageMask = VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT;
            break;

        default:
            YG_CORE_ERROR("Vulkan: Unexpected image layout to pipeline stage mapping");
            break;
    }

    return StageMask;
}

VkShaderStageFlagBits YgShaderStage2VkShaderStage(ShaderStage stage)
{
    switch (stage)
    {
        case ShaderStage::Vertex:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case ShaderStage::Fragment:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case ShaderStage::Compute:
            return VK_SHADER_STAGE_COMPUTE_BIT;
        case ShaderStage::Geometry:
            return VK_SHADER_STAGE_GEOMETRY_BIT;
        default:
            YG_CORE_ERROR("Vulkan: Unsupported shader stage!");
            return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
    }
}

PFN_vkVoidFunction VkLoadFunction(const char* funcName, void* instance)
{
    return vkGetInstanceProcAddr((VkInstance)instance, funcName);
}

} // namespace Yogi
