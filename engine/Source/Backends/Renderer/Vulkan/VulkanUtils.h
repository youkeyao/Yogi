#pragma once

#include <volk.h>
#ifdef YG_PLATFORM_WINDOWS
#    include <vulkan/vulkan_win32.h>
#elif YG_PLATFORM_MACOS
#    include <vulkan/vulkan_metal.h>
#elif YG_PLATFORM_LINUX
#    include <vulkan/vulkan_xlib.h>
#    include <vulkan/vulkan_xcb.h>
#endif

#include "Core/Window.h"
#include "Renderer/RHI/ITexture.h"
#include "Renderer/RHI/IRenderPass.h"
#include "Renderer/RHI/IPipeline.h"

namespace Yogi
{

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
};
bool               CheckValidationLayerSupport(const std::vector<const char*>& validationLayers);
QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
bool CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& deviceExtensions);
bool IsDeviceSuitable(VkPhysicalDevice device, const std::vector<const char*>& deviceExtensions);

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR        capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;
};
SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
VkPresentModeKHR        ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
VkExtent2D              ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, const View<Window>& window);

uint32_t FindMemoryType(uint32_t typeFilter, VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags properties);

VkFormat              YgTextureFormat2VkFormat(ITexture::Format format);
VkFormat              YgShaderElementType2VkFormat(ShaderElementType type);
VkImageLayout         AttachmentUsage2VkImageLayout(AttachmentUsage usage);
VkAccessFlags         AccessMaskFromImageLayout(VkImageLayout Layout, bool IsDstMask);
VkPipelineStageFlags  PipelineStageFromImageLayout(VkImageLayout Layout, bool IsDstStage);
VkShaderStageFlagBits YgShaderStage2VkShaderStage(ShaderStage stage);

} // namespace Yogi