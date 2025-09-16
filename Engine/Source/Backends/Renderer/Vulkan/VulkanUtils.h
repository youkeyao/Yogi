#pragma once

#include "Core/Window.h"
#include "Renderer/RHI/ITexture.h"
#include "Renderer/RHI/IRenderPass.h"
#include "Renderer/RHI/IPipeline.h"

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
VkExtent2D              ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, const Ref<Window>& window);

uint32_t FindMemoryType(uint32_t typeFilter, VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags properties);

VkFormat              YgTextureFormat2VkFormat(ITexture::Format format);
ITexture::Format      VkFormat2YgTextureFormat(VkFormat format);
VkFormat              YgShaderElementType2VkFormat(ShaderElementType type);
VkImageLayout         AttachmentUsage2VkImageLayout(AttachmentUsage usage);
VkAccessFlags         AccessMaskFromImageLayout(VkImageLayout Layout, bool IsDstMask);
VkPipelineStageFlags  PipelineStageFromImageLayout(VkImageLayout Layout, bool IsDstStage);
VkShaderStageFlagBits YgShaderStage2VkShaderStage(ShaderStage stage);

YG_API PFN_vkVoidFunction VkLoadFunction(const char* funcName, void* instance);

} // namespace Yogi