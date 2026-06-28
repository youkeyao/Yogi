#pragma once

#include "Core/Window.h"
#include "Renderer/RHI/ITexture.h"
#include "Renderer/RHI/IPipeline.h"
#include "Renderer/RHI/ICommandBuffer.h"
#include "Renderer/RHI/ISampler.h"

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

namespace Yogi
{

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> computeFamily;
    std::optional<uint32_t> transferFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && computeFamily.has_value() && transferFamily.has_value() &&
            presentFamily.has_value();
    }
};
bool                      CheckValidationLayerSupport(const std::vector<const char*>& validationLayers);
YG_API QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
bool CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& deviceExtensions);
bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<const char*>& deviceExtensions);

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR        capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;
};
SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
VkPresentModeKHR        ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
VkExtent2D              ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, const Window& window);

uint32_t FindMemoryType(uint32_t typeFilter, VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags properties);

YG_API VkFormat       YgFormat2VkFormat(Format format);
Format                VkFormat2YgFormat(VkFormat format);
VkImageLayout         YgResourceState2VkImageLayout(ResourceState state, Format format);
VkShaderStageFlags    YgShaderStage2VkShaderStage(ShaderStage stage);
VkPrimitiveTopology   YgPrimitiveTopology2VkPrimitiveTopology(PrimitiveTopology topology);
VkImageUsageFlags     YgTextureUsageFlags2VkImageUsage(TextureUsageFlags flags);
VkBlendFactor         YgBlendFactor2Vk(BlendFactor factor);
VkBlendOp             YgBlendOp2Vk(BlendOp op);
VkColorComponentFlags YgColorWriteMask2Vk(ColorWriteMask mask);

bool     YgTextureFormatHasStencil(Format format);
bool     YgTextureFormatIsDepthStencil(Format format);
uint32_t YgTextureFormatBytesPerPixel(Format format);

// Creates a VkSampler from a SamplerDesc. Caller owns the returned handle and must vkDestroySampler it.
VkSampler YgCreateVkSampler(VkDevice device, const SamplerDesc& desc);

VkPipelineStageFlags2 YgResourceState2VkPipelineStage2(ResourceState state);
VkAccessFlags2        YgResourceState2VkAccess2(ResourceState state);

YG_API PFN_vkVoidFunction VkLoadFunction(const char* funcName, void* instance);

} // namespace Yogi