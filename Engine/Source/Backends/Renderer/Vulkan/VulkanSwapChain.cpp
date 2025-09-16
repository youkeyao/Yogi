#include "VulkanSwapChain.h"
#include "VulkanCommandBuffer.h"

#include <volk.h>

namespace Yogi
{

Handle<ISwapChain> ISwapChain::Create(const SwapChainDesc& desc) { return Handle<VulkanSwapChain>::Create(desc); }

VulkanSwapChain::VulkanSwapChain(const SwapChainDesc& desc) :
    m_width(desc.Width),
    m_height(desc.Height),
    m_colorFormat(desc.ColorFormat),
    m_depthFormat(desc.DepthFormat),
    m_numSamples(desc.NumSamples),
    m_window(desc.Window)
{
    CreateVkSwapChain();
    CreateVkSyncObjects();
}

VulkanSwapChain::~VulkanSwapChain()
{
    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext().Get());

    CleanupSwapChain();
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroySemaphore(context->GetVkDevice(), m_imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(context->GetVkDevice(), m_renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(context->GetVkDevice(), m_renderCommandFences[i], nullptr);
    }
}

void VulkanSwapChain::AcquireNextImage()
{
    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext().Get());
    VkDevice             device  = context->GetVkDevice();

    vkWaitForFences(device, 1, &m_renderCommandFences[m_currentFrame], VK_TRUE, UINT64_MAX);

    VkResult result = vkAcquireNextImageKHR(
        device, m_swapChain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &m_imageIndex);
    YG_CORE_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Vulkan: Failed to acquire swap chain image!");

    vkResetFences(device, 1, &m_renderCommandFences[m_currentFrame]);
}

void VulkanSwapChain::Present()
{
    YG_PROFILE_FUNCTION();

    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext().Get());
    VkDevice             device  = context->GetVkDevice();

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &m_imageAvailableSemaphores[m_currentFrame];

    VkSwapchainKHR swapChains[] = { m_swapChain };
    presentInfo.swapchainCount  = 1;
    presentInfo.pSwapchains     = swapChains;
    presentInfo.pImageIndices   = &m_imageIndex;
    presentInfo.pResults        = nullptr;

    VkResult result = vkQueuePresentKHR(m_presentQueue, &presentInfo);
    YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to present swap chain image!");

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanSwapChain::Resize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
        return;

    m_width  = width;
    m_height = height;

    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext().Get());
    vkDeviceWaitIdle(context->GetVkDevice());

    CleanupSwapChain();
    CreateVkSwapChain();
}

// ---------------------------------------------------------------------------------------------

void VulkanSwapChain::CleanupSwapChain()
{
    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext().Get());

    for (auto& texture : m_colorTextures)
    {
        texture = nullptr;
    }
    m_colorTextures.clear();

    for (auto& texture : m_depthTextures)
    {
        texture = nullptr;
    }
    m_depthTextures.clear();

    if (m_swapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(context->GetVkDevice(), m_swapChain, nullptr);
        m_swapChain = VK_NULL_HANDLE;
    }
}

void VulkanSwapChain::CreateVkSwapChain()
{
    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext().Get());
    VkPhysicalDevice     physicalDevice = context->GetVkPhysicalDevice();
    VkDevice             device         = context->GetVkDevice();
    VkSurfaceKHR         surface        = context->GetVkSurface();

    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice, surface);
    VkSurfaceFormatKHR      surfaceFormat    = swapChainSupport.formats[0];
    VkFormat                targetFormat     = YgTextureFormat2VkFormat(m_colorFormat);
    for (const auto& availableFormat : swapChainSupport.formats)
    {
        if (availableFormat.format == targetFormat && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            surfaceFormat = availableFormat;
            break;
        }
    }
    m_colorFormat                = VkFormat2YgTextureFormat(surfaceFormat.format);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D       extent      = ChooseSwapExtent(swapChainSupport.capabilities, m_window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;

    createInfo.minImageCount    = imageCount;
    createInfo.imageFormat      = surfaceFormat.format;
    createInfo.imageColorSpace  = surfaceFormat.colorSpace;
    createInfo.imageExtent      = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices              = FindQueueFamilies(physicalDevice, surface);
    uint32_t           queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform   = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode    = presentMode;
    createInfo.clipped        = VK_TRUE;

    VkResult result = vkCreateSwapchainKHR(device, &createInfo, nullptr, &m_swapChain);
    YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to create swap chain!");

    vkGetSwapchainImagesKHR(device, m_swapChain, &imageCount, nullptr);
    std::vector<VkImage> swapChainImages(imageCount);
    vkGetSwapchainImagesKHR(device, m_swapChain, &imageCount, swapChainImages.data());

    m_colorTextures.reserve(imageCount);
    m_colorTextures.clear();
    for (size_t i = 0; i < swapChainImages.size(); ++i)
    {
        m_colorTextures.emplace_back(Handle<VulkanTexture>::Create(
            extent.width, extent.height, m_colorFormat, ITexture::Usage::RenderTarget, swapChainImages[i]));
    }

    m_depthTextures.reserve(imageCount);
    m_depthTextures.clear();
    for (size_t i = 0; i < swapChainImages.size(); ++i)
    {
        m_depthTextures.emplace_back(Handle<VulkanTexture>::Create(
            TextureDesc{ extent.width, extent.height, 1, m_depthFormat, ITexture::Usage::DepthStencil, m_numSamples }));
    }
}

void VulkanSwapChain::CreateVkSyncObjects()
{
    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext().Get());
    VkPhysicalDevice     physicalDevice = context->GetVkPhysicalDevice();
    VkDevice             device         = context->GetVkDevice();

    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderCommandFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        YG_CORE_ASSERT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) == VK_SUCCESS,
                       "Vulkan: Failed to create image available semaphore!");
        YG_CORE_ASSERT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) == VK_SUCCESS,
                       "Vulkan: Failed to create render finished semaphore!");
        YG_CORE_ASSERT(vkCreateFence(device, &fenceInfo, nullptr, &m_renderCommandFences[i]) == VK_SUCCESS,
                       "Vulkan: Failed to create in-flight fence!");
    }

    QueueFamilyIndices indices = FindQueueFamilies(physicalDevice, context->GetVkSurface());
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &m_presentQueue);
}

} // namespace Yogi
