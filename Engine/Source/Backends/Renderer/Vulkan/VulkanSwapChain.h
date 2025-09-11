#pragma once

#include "Renderer/RHI/ISwapChain.h"
#include "VulkanTexture.h"

namespace Yogi
{

class VulkanSwapChain : public ISwapChain
{
public:
    VulkanSwapChain(const SwapChainDesc& desc);
    virtual ~VulkanSwapChain();

    inline uint32_t            GetWidth() const override { return m_width; }
    inline uint32_t            GetHeight() const override { return m_height; }
    inline ITexture::Format    GetColorFormat() const override { return m_colorFormat; }
    inline ITexture::Format    GetDepthFormat() const override { return m_depthFormat; }
    inline SampleCountFlagBits GetNumSamples() const override { return m_numSamples; }

    Ref<ITexture> GetCurrentTarget() const override
    {
        return Ref<VulkanTexture>::Create(m_colorTextures[m_imageIndex]);
    }
    Ref<ITexture> GetCurrentDepth() const override { return Ref<VulkanTexture>::Create(m_depthTextures[m_imageIndex]); }

    void AcquireNextImage() override;
    void Present() override;
    void Resize(uint32_t width, uint32_t height) override;

    inline VkFence GetVkRenderCommandFence() const { return m_renderCommandFences[m_currentFrame]; }

private:
    void CleanupSwapChain();
    void RecreateSwapChain();
    void CreateVkSurface();
    void CreateVkSwapChain();
    void CreateVkSyncObjects();

private:
    const int MAX_FRAMES_IN_FLIGHT = 2;

    uint32_t m_currentFrame = 0;
    uint32_t m_imageIndex   = 0;

    VkSurfaceKHR   m_surface      = VK_NULL_HANDLE;
    VkSwapchainKHR m_swapChain    = VK_NULL_HANDLE;
    VkQueue        m_presentQueue = VK_NULL_HANDLE;

    std::vector<Handle<VulkanTexture>> m_colorTextures;
    std::vector<Handle<VulkanTexture>> m_depthTextures;

    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence>     m_renderCommandFences;

    uint32_t            m_width;
    uint32_t            m_height;
    ITexture::Format    m_colorFormat;
    ITexture::Format    m_depthFormat;
    SampleCountFlagBits m_numSamples;
    Ref<Window>         m_window;
};

} // namespace Yogi