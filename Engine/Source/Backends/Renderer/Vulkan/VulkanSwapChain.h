#pragma once

#include "Renderer/RHI/ISwapChain.h"
#include "VulkanTexture.h"
#include "VulkanCommandBuffer.h"

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
    inline SampleCountFlagBits GetNumSamples() const override { return m_numSamples; }

    ITexture*       GetCurrentTarget() const override { return m_colorTextures[m_imageIndex].Get(); }
    ICommandBuffer* GetCurrentCommandBuffer() const override { return m_commandBuffers[m_currentFrame].Get(); }

    void AcquireNextImage() override;
    void Present() override;
    void Resize(uint32_t width, uint32_t height) override;

    inline uint32_t GetImageCount() const { return m_colorTextures.size(); }

private:
    void CleanupSwapChain();
    void RecreateSwapChain();
    void CreateVkSwapChain();
    void CreateVkSyncObjects();

private:
    uint32_t m_currentFrame = 0;
    uint32_t m_imageIndex   = 0;

    VkSwapchainKHR m_swapChain    = VK_NULL_HANDLE;
    VkQueue        m_presentQueue = VK_NULL_HANDLE;

    std::vector<Owner<VulkanTexture>> m_colorTextures;

    std::vector<VkSemaphore>                m_imageAvailableSemaphores;
    std::vector<Owner<VulkanCommandBuffer>> m_commandBuffers;

    uint32_t            m_width;
    uint32_t            m_height;
    ITexture::Format    m_colorFormat;
    SampleCountFlagBits m_numSamples;
    Window*             m_window = nullptr;
};

} // namespace Yogi