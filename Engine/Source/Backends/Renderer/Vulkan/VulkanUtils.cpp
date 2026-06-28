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
VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, const Window& window)
{
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }
    else
    {
        int width  = window.GetWidth();
        int height = window.GetHeight();

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

VkFormat YgFormat2VkFormat(Format format)
{
    switch (format)
    {
        case Format::R8_UNORM:
            return VK_FORMAT_R8_UNORM;
        case Format::R8G8_UNORM:
            return VK_FORMAT_R8G8_UNORM;
        case Format::R8G8B8_UNORM:
            return VK_FORMAT_R8G8B8_UNORM;
        case Format::R8G8B8_SRGB:
            return VK_FORMAT_R8G8B8_SRGB;
        case Format::R8G8B8A8_UNORM:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case Format::R8G8B8A8_SRGB:
            return VK_FORMAT_R8G8B8A8_SRGB;
        case Format::B8G8R8A8_UNORM:
            return VK_FORMAT_B8G8R8A8_UNORM;
        case Format::B8G8R8A8_SRGB:
            return VK_FORMAT_B8G8R8A8_SRGB;
        case Format::R16_FLOAT:
            return VK_FORMAT_R16_SFLOAT;
        case Format::R16G16_FLOAT:
            return VK_FORMAT_R16G16_SFLOAT;
        case Format::R16G16B16A16_FLOAT:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        case Format::R11G11B10_FLOAT:
            return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
        case Format::R32_FLOAT:
            return VK_FORMAT_R32_SFLOAT;
        case Format::R32G32_FLOAT:
            return VK_FORMAT_R32G32_SFLOAT;
        case Format::R32G32B32_FLOAT:
            return VK_FORMAT_R32G32B32_SFLOAT;
        case Format::R32G32B32A32_FLOAT:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        case Format::R32_UINT:
            return VK_FORMAT_R32_UINT;
        case Format::R32G32_UINT:
            return VK_FORMAT_R32G32_UINT;
        case Format::R32G32B32A32_UINT:
            return VK_FORMAT_R32G32B32A32_UINT;
        case Format::D16_UNORM:
            return VK_FORMAT_D16_UNORM;
        case Format::D32_FLOAT:
            return VK_FORMAT_D32_SFLOAT;
        case Format::D24_UNORM_S8_UINT:
            return VK_FORMAT_D24_UNORM_S8_UINT;
        default:
            YG_CORE_ERROR("Vulkan: Unsupported Yg texture format!");
            return VK_FORMAT_UNDEFINED;
    }
}

Format VkFormat2YgFormat(VkFormat format)
{
    switch (format)
    {
        case VK_FORMAT_R8_UNORM:
            return Format::R8_UNORM;
        case VK_FORMAT_R8G8_UNORM:
            return Format::R8G8_UNORM;
        case VK_FORMAT_R8G8B8_UNORM:
            return Format::R8G8B8_UNORM;
        case VK_FORMAT_R8G8B8_SRGB:
            return Format::R8G8B8_SRGB;
        case VK_FORMAT_R8G8B8A8_UNORM:
            return Format::R8G8B8A8_UNORM;
        case VK_FORMAT_R8G8B8A8_SRGB:
            return Format::R8G8B8A8_SRGB;
        case VK_FORMAT_B8G8R8A8_UNORM:
            return Format::B8G8R8A8_UNORM;
        case VK_FORMAT_B8G8R8A8_SRGB:
            return Format::B8G8R8A8_SRGB;
        case VK_FORMAT_R16_SFLOAT:
            return Format::R16_FLOAT;
        case VK_FORMAT_R16G16_SFLOAT:
            return Format::R16G16_FLOAT;
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return Format::R16G16B16A16_FLOAT;
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
            return Format::R11G11B10_FLOAT;
        case VK_FORMAT_R32_SFLOAT:
            return Format::R32_FLOAT;
        case VK_FORMAT_R32G32_SFLOAT:
            return Format::R32G32_FLOAT;
        case VK_FORMAT_R32G32B32_SFLOAT:
            return Format::R32G32B32_FLOAT;
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return Format::R32G32B32A32_FLOAT;
        case VK_FORMAT_R32_UINT:
            return Format::R32_UINT;
        case VK_FORMAT_R32G32_UINT:
            return Format::R32G32_UINT;
        case VK_FORMAT_R32G32B32A32_UINT:
            return Format::R32G32B32A32_UINT;
        case VK_FORMAT_D16_UNORM:
            return Format::D16_UNORM;
        case VK_FORMAT_D32_SFLOAT:
            return Format::D32_FLOAT;
        case VK_FORMAT_D24_UNORM_S8_UINT:
            return Format::D24_UNORM_S8_UINT;
        default:
            YG_CORE_ERROR("Vulkan: Unsupported Vk texture format!");
            return Format::NONE;
    }
}

VkImageLayout YgResourceState2VkImageLayout(ResourceState state, Format format)
{
    if (state & ResourceState::Undefined)
        return VK_IMAGE_LAYOUT_UNDEFINED;
    if (state & ResourceState::Present)
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    if (state & ResourceState::UnorderedAccess)
        return VK_IMAGE_LAYOUT_GENERAL;
    if (state & ResourceState::CopyDestination)
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    if (state & ResourceState::CopySource)
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    if (state & ResourceState::ColorAttachment)
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    if (state & ResourceState::DepthWrite)
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    if (state & ResourceState::DepthRead)
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    if (state & ResourceState::VertexShaderResource || state & ResourceState::FragmentShaderResource ||
        state & ResourceState::ComputeShaderResource || state & ResourceState::TaskShaderResource ||
        state & ResourceState::MeshShaderResource)
    {
        return YgTextureFormatIsDepthStencil(format) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL :
                                                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    return VK_IMAGE_LAYOUT_UNDEFINED;
}

VkAccessFlags2 YgResourceState2VkAccess2(ResourceState state)
{
    VkAccessFlags2 flags = 0;

    if (state & ResourceState::VertexShaderResource || state & ResourceState::FragmentShaderResource ||
        state & ResourceState::ComputeShaderResource || state & ResourceState::TaskShaderResource ||
        state & ResourceState::MeshShaderResource)
        flags |= VK_ACCESS_2_SHADER_READ_BIT;
    if (state & ResourceState::UnorderedAccess)
        flags |= VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
    if (state & ResourceState::UniformBuffer)
        flags |= VK_ACCESS_2_UNIFORM_READ_BIT;
    if (state & ResourceState::IndirectArg)
        flags |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
    if (state & ResourceState::CopySource)
        flags |= VK_ACCESS_2_TRANSFER_READ_BIT;
    if (state & ResourceState::CopyDestination)
        flags |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
    if (state & ResourceState::ColorAttachment)
        flags |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    if (state & ResourceState::DepthRead)
        flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    if (state & ResourceState::DepthWrite)
        flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    if (state & ResourceState::VertexBuffer)
        flags |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
    if (state & ResourceState::IndexBuffer)
        flags |= VK_ACCESS_2_INDEX_READ_BIT;

    return flags;
}

VkPipelineStageFlags2 YgResourceState2VkPipelineStage2(ResourceState state)
{
    VkPipelineStageFlags2 flags = 0;

    if (state & ResourceState::IndirectArg)
        flags |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
    if (state & ResourceState::VertexBuffer || state & ResourceState::IndexBuffer)
        flags |= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
    if (state & ResourceState::VertexShaderResource)
        flags |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
    if (state & ResourceState::FragmentShaderResource)
        flags |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    if (state & ResourceState::ComputeShaderResource)
        flags |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    if (state & ResourceState::TaskShaderResource)
        flags |= VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT;
    if (state & ResourceState::MeshShaderResource)
        flags |= VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT;
    if (state & ResourceState::UnorderedAccess)
    {
        flags |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT |
            VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT;
    }
    if (state & ResourceState::UniformBuffer)
    {
        flags |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT |
            VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT;
    }
    if (state & ResourceState::CopySource || state & ResourceState::CopyDestination)
        flags |= VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
    if (state & ResourceState::ColorAttachment)
        flags |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    if (state & ResourceState::DepthRead || state & ResourceState::DepthWrite)
        flags |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;

    return flags == 0 ? VK_PIPELINE_STAGE_2_NONE : flags;
}

VkShaderStageFlags YgShaderStage2VkShaderStage(ShaderStage stage)
{
    VkShaderStageFlags flags = 0;
    if (static_cast<uint8_t>(stage & ShaderStage::Vertex))
        flags |= VK_SHADER_STAGE_VERTEX_BIT;
    if (static_cast<uint8_t>(stage & ShaderStage::Geometry))
        flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
    if (static_cast<uint8_t>(stage & ShaderStage::Fragment))
        flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
    if (static_cast<uint8_t>(stage & ShaderStage::Compute))
        flags |= VK_SHADER_STAGE_COMPUTE_BIT;
    if (static_cast<uint8_t>(stage & ShaderStage::Task))
        flags |= VK_SHADER_STAGE_TASK_BIT_EXT;
    if (static_cast<uint8_t>(stage & ShaderStage::Mesh))
        flags |= VK_SHADER_STAGE_MESH_BIT_EXT;
    return flags;
}

VkPrimitiveTopology YgPrimitiveTopology2VkPrimitiveTopology(PrimitiveTopology topology)
{
    switch (topology)
    {
        case PrimitiveTopology::TriangleList:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case PrimitiveTopology::TriangleStrip:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case PrimitiveTopology::LineList:
            return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case PrimitiveTopology::PointList:
            return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        default:
            YG_CORE_ERROR("Vulkan: Unsupported primitive topology!");
            return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
    }
}

PFN_vkVoidFunction VkLoadFunction(const char* funcName, void* instance)
{
    return vkGetInstanceProcAddr((VkInstance)instance, funcName);
}

bool YgTextureFormatHasStencil(Format format)
{
    switch (format)
    {
        case Format::D24_UNORM_S8_UINT:
            return true;
        default:
            return false;
    }
}

bool YgTextureFormatIsDepthStencil(Format format)
{
    switch (format)
    {
        case Format::D16_UNORM:
        case Format::D32_FLOAT:
        case Format::D24_UNORM_S8_UINT:
            return true;
        default:
            return false;
    }
}

uint32_t YgTextureFormatBytesPerPixel(Format format)
{
    switch (format)
    {
        case Format::R8_UNORM:
            return 1;
        case Format::R8G8_UNORM:
        case Format::R16_FLOAT:
        case Format::D16_UNORM:
            return 2;
        case Format::R8G8B8_UNORM:
        case Format::R8G8B8_SRGB:
            return 3;
        case Format::R8G8B8A8_UNORM:
        case Format::R8G8B8A8_SRGB:
        case Format::B8G8R8A8_UNORM:
        case Format::B8G8R8A8_SRGB:
        case Format::R16G16_FLOAT:
        case Format::R11G11B10_FLOAT:
        case Format::R32_FLOAT:
        case Format::R32_UINT:
        case Format::D32_FLOAT:
        case Format::D24_UNORM_S8_UINT:
            return 4;
        case Format::R16G16B16A16_FLOAT:
        case Format::R32G32_UINT:
            return 8;
        case Format::R32G32B32_FLOAT:
            return 12;
        case Format::R32G32B32A32_FLOAT:
        case Format::R32G32B32A32_UINT:
            return 16;
        default:
            YG_CORE_ERROR("Vulkan: BytesPerPixel for unsupported format!");
            return 0;
    }
}

VkImageUsageFlags YgTextureUsageFlags2VkImageUsage(TextureUsageFlags flags)
{
    VkImageUsageFlags result = 0;
    if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(TextureUsageFlags::Sampled))
        result |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(TextureUsageFlags::ColorAttachment))
        result |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(TextureUsageFlags::DepthStencil))
        result |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(TextureUsageFlags::Storage))
        result |= VK_IMAGE_USAGE_STORAGE_BIT;
    if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(TextureUsageFlags::TransferSrc))
        result |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(TextureUsageFlags::TransferDst))
        result |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    return result;
}

VkBlendFactor YgBlendFactor2Vk(BlendFactor factor)
{
    switch (factor)
    {
        case BlendFactor::Zero:
            return VK_BLEND_FACTOR_ZERO;
        case BlendFactor::One:
            return VK_BLEND_FACTOR_ONE;
        case BlendFactor::SrcColor:
            return VK_BLEND_FACTOR_SRC_COLOR;
        case BlendFactor::OneMinusSrcColor:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case BlendFactor::DstColor:
            return VK_BLEND_FACTOR_DST_COLOR;
        case BlendFactor::OneMinusDstColor:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case BlendFactor::SrcAlpha:
            return VK_BLEND_FACTOR_SRC_ALPHA;
        case BlendFactor::OneMinusSrcAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case BlendFactor::DstAlpha:
            return VK_BLEND_FACTOR_DST_ALPHA;
        case BlendFactor::OneMinusDstAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        default:
            return VK_BLEND_FACTOR_ZERO;
    }
}

VkBlendOp YgBlendOp2Vk(BlendOp op)
{
    switch (op)
    {
        case BlendOp::Add:
            return VK_BLEND_OP_ADD;
        case BlendOp::Subtract:
            return VK_BLEND_OP_SUBTRACT;
        case BlendOp::ReverseSubtract:
            return VK_BLEND_OP_REVERSE_SUBTRACT;
        case BlendOp::Min:
            return VK_BLEND_OP_MIN;
        case BlendOp::Max:
            return VK_BLEND_OP_MAX;
        default:
            return VK_BLEND_OP_ADD;
    }
}

VkColorComponentFlags YgColorWriteMask2Vk(ColorWriteMask mask)
{
    VkColorComponentFlags result = 0;
    if (static_cast<uint32_t>(mask) & static_cast<uint32_t>(ColorWriteMask::R))
        result |= VK_COLOR_COMPONENT_R_BIT;
    if (static_cast<uint32_t>(mask) & static_cast<uint32_t>(ColorWriteMask::G))
        result |= VK_COLOR_COMPONENT_G_BIT;
    if (static_cast<uint32_t>(mask) & static_cast<uint32_t>(ColorWriteMask::B))
        result |= VK_COLOR_COMPONENT_B_BIT;
    if (static_cast<uint32_t>(mask) & static_cast<uint32_t>(ColorWriteMask::A))
        result |= VK_COLOR_COMPONENT_A_BIT;
    return result;
}

VkSampler YgCreateVkSampler(VkDevice device, const SamplerDesc& desc)
{
    auto toFilter = [](Filter f) {
        return f == Filter::Nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
    };
    auto toMip = [](MipmapMode m) {
        return m == MipmapMode::Nearest ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
    };
    auto toAddr = [](SamplerAddressMode m) {
        switch (m)
        {
            case SamplerAddressMode::Repeat:
                return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            case SamplerAddressMode::MirroredRepeat:
                return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            case SamplerAddressMode::ClampToEdge:
                return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case SamplerAddressMode::ClampToBorder:
                return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            default:
                return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }
    };

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter    = toFilter(desc.MagFilter);
    samplerInfo.minFilter    = toFilter(desc.MinFilter);
    samplerInfo.mipmapMode   = toMip(desc.MipMode);
    samplerInfo.addressModeU = toAddr(desc.AddressU);
    samplerInfo.addressModeV = toAddr(desc.AddressV);
    samplerInfo.addressModeW = toAddr(desc.AddressW);

    const bool anisoEnabled      = desc.MaxAnisotropy > 1.0f;
    samplerInfo.anisotropyEnable = anisoEnabled ? VK_TRUE : VK_FALSE;
    samplerInfo.maxAnisotropy    = anisoEnabled ? desc.MaxAnisotropy : 1.0f;

    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp     = VK_COMPARE_OP_ALWAYS;
    samplerInfo.minLod        = desc.MinLod;
    samplerInfo.maxLod        = desc.MaxLod;
    samplerInfo.borderColor   = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

    VkSamplerReductionModeCreateInfo reductionInfo{};
    if (desc.Reduction != SamplerReductionMode::None)
    {
        reductionInfo.sType         = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO;
        reductionInfo.reductionMode = (desc.Reduction == SamplerReductionMode::Max) ? VK_SAMPLER_REDUCTION_MODE_MAX :
                                                                                      VK_SAMPLER_REDUCTION_MODE_MIN;
        samplerInfo.pNext           = &reductionInfo;
    }

    VkSampler sampler = VK_NULL_HANDLE;
    VkResult  result  = vkCreateSampler(device, &samplerInfo, nullptr, &sampler);
    YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to create sampler!");
    return sampler;
}

} // namespace Yogi
