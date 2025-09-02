
#pragma once

#include "Renderer/RHI/IFrameBuffer.h"
#include "VulkanRenderPass.h"
#include "VulkanTexture.h"

namespace Yogi
{

class VulkanFrameBuffer : public IFrameBuffer
{
public:
    VulkanFrameBuffer(const FrameBufferDesc& desc);
    virtual ~VulkanFrameBuffer();

    uint32_t                           GetWidth() const override { return m_width; }
    uint32_t                           GetHeight() const override { return m_height; }
    View<IRenderPass>                  GetRenderPass() const override { return m_renderPass; }
    const std::vector<View<ITexture>>& GetColorAttachments() const override { return m_colorAttachments; }
    View<ITexture>                     GetDepthAttachment() const override { return m_depthAttachment; }

    inline VkFramebuffer GetVkFrameBuffer() const { return m_frameBuffer; }

private:
    void Cleanup();
    void CreateVkFrameBuffer();

private:
    VkFramebuffer m_frameBuffer = VK_NULL_HANDLE;

    std::vector<Scope<VulkanTexture>> m_msaaTextures;

    uint32_t                    m_width;
    uint32_t                    m_height;
    View<IRenderPass>           m_renderPass = nullptr;
    std::vector<View<ITexture>> m_colorAttachments;
    View<ITexture>              m_depthAttachment = nullptr;
};

} // namespace Yogi
