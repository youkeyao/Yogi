#include "VulkanSwapChain.h"
#include "VulkanCommandBuffer.h"

#ifdef YG_WINDOW_GLFW
#    include <GLFW/glfw3.h>
#endif

namespace Yogi
{

Scope<ISwapChain> ISwapChain::Create(const SwapChainDesc& desc) { return CreateScope<VulkanSwapChain>(desc); }

VulkanSwapChain::VulkanSwapChain(const SwapChainDesc& desc) :
    m_width(desc.Width),
    m_height(desc.Height),
    m_colorFormat(desc.ColorFormat),
    m_depthFormat(desc.DepthFormat),
    m_numSamples(desc.NumSamples),
    m_window(desc.Window)
{
    CreateVkSurface();
    CreateVkSwapChain();
    CreateVkSyncObjects();
}

VulkanSwapChain::~VulkanSwapChain()
{
    View<VulkanDeviceContext> deviceContext =
        static_cast<View<VulkanDeviceContext>>(Application::GetInstance().GetContext());

    Cleanup();
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroySemaphore(deviceContext->GetVkDevice(), m_imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(deviceContext->GetVkDevice(), m_renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(deviceContext->GetVkDevice(), m_renderCommandFences[i], nullptr);
    }
    vkDestroySurfaceKHR(deviceContext->GetVkInstance(), m_surface, nullptr);
}

void VulkanSwapChain::AcquireNextImage()
{
    View<VulkanDeviceContext> deviceContext =
        static_cast<View<VulkanDeviceContext>>(Application::GetInstance().GetContext());
    VkDevice device = deviceContext->GetVkDevice();

    vkWaitForFences(device, 1, &m_renderCommandFences[m_currentFrame], VK_TRUE, UINT64_MAX);

    VkResult result = vkAcquireNextImageKHR(
        device, m_swapChain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &m_imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        int width  = m_window->GetWidth();
        int height = m_window->GetHeight();
        Resize(width, height);
    }
    else
    {
        YG_CORE_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR,
                       "Vulkan: Failed to acquire swap chain image!");
    }

    vkResetFences(device, 1, &m_renderCommandFences[m_currentFrame]);
}

void VulkanSwapChain::Present()
{
    View<VulkanDeviceContext> deviceContext =
        static_cast<View<VulkanDeviceContext>>(Application::GetInstance().GetContext());
    VkDevice device = deviceContext->GetVkDevice();

    VulkanCommandBuffer submitCommandBuffer(
        CommandBufferDesc{ CommandBufferUsage::OneTimeSubmit, SubmitQueue::Graphics });
    vkWaitForFences(device, 1, &m_renderCommandFences[m_currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &m_renderCommandFences[m_currentFrame]);
    submitCommandBuffer.Begin();
    View<VulkanTexture> target = static_cast<View<VulkanTexture>>(CreateView(m_colorTextures[m_imageIndex]));
    submitCommandBuffer.TransitionImageLayout(target->GetVkImage(),
                                              ITexture::Usage::RenderTarget,
                                              VK_IMAGE_LAYOUT_UNDEFINED,
                                              VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    submitCommandBuffer.End();
    submitCommandBuffer.Submit();

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

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        int width  = m_window->GetWidth();
        int height = m_window->GetHeight();
        Resize(width, height);
    }
    else
    {
        YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to present swap chain image!");
    }

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanSwapChain::Resize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
        return;

    m_width  = width;
    m_height = height;

    View<VulkanDeviceContext> deviceContext =
        static_cast<View<VulkanDeviceContext>>(Application::GetInstance().GetContext());
    vkDeviceWaitIdle(deviceContext->GetVkDevice());

    Cleanup();
    CreateVkSwapChain();
}

// ---------------------------------------------------------------------------------------------

void VulkanSwapChain::Cleanup()
{
    View<VulkanDeviceContext> deviceContext =
        static_cast<View<VulkanDeviceContext>>(Application::GetInstance().GetContext());

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
        vkDestroySwapchainKHR(deviceContext->GetVkDevice(), m_swapChain, nullptr);
        m_swapChain = VK_NULL_HANDLE;
    }
}

void VulkanSwapChain::CreateVkSurface()
{
    View<VulkanDeviceContext> deviceContext =
        static_cast<View<VulkanDeviceContext>>(Application::GetInstance().GetContext());

    if (m_surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(deviceContext->GetVkInstance(), m_surface, NULL);
        m_surface = VK_NULL_HANDLE;
    }

    VkResult result = VK_ERROR_INITIALIZATION_FAILED;
#ifdef YG_WINDOW_GLFW
    result = glfwCreateWindowSurface(
        deviceContext->GetVkInstance(), (GLFWwindow*)m_window->GetNativeWindow(), nullptr, &m_surface);
#endif

    YG_CORE_ASSERT(result == VK_SUCCESS && m_surface != VK_NULL_HANDLE, "Vulkan: Failed to create window surface!");
}

void VulkanSwapChain::CreateVkSwapChain()
{
    View<VulkanDeviceContext> deviceContext =
        static_cast<View<VulkanDeviceContext>>(Application::GetInstance().GetContext());
    VkPhysicalDevice physicalDevice = deviceContext->GetVkPhysicalDevice();
    VkDevice         device         = deviceContext->GetVkDevice();

    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice, m_surface);
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
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D       extent      = ChooseSwapExtent(swapChainSupport.capabilities, m_window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;

    createInfo.minImageCount    = imageCount;
    createInfo.imageFormat      = surfaceFormat.format;
    createInfo.imageColorSpace  = surfaceFormat.colorSpace;
    createInfo.imageExtent      = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices              = FindQueueFamilies(physicalDevice);
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

    m_colorTextures.resize(imageCount);
    for (size_t i = 0; i < swapChainImages.size(); ++i)
    {
        m_colorTextures[i] = CreateScope<VulkanTexture>(
            extent.width, extent.height, m_colorFormat, ITexture::Usage::RenderTarget, swapChainImages[i]);
    }

    m_depthTextures.resize(imageCount);
    for (size_t i = 0; i < swapChainImages.size(); ++i)
    {
        m_depthTextures[i] = CreateScope<VulkanTexture>(Yogi::TextureDesc{
            extent.width, extent.height, 1, m_depthFormat, Yogi::ITexture::Usage::DepthStencil, m_numSamples });
    }
}

void VulkanSwapChain::CreateVkSyncObjects()
{
    View<VulkanDeviceContext> deviceContext =
        static_cast<View<VulkanDeviceContext>>(Application::GetInstance().GetContext());
    VkPhysicalDevice physicalDevice = deviceContext->GetVkPhysicalDevice();
    VkDevice         device         = deviceContext->GetVkDevice();

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

    QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &m_presentQueue);
}

} // namespace Yogi
