
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

    inline uint32_t                          GetWidth() const override { return m_width; }
    inline uint32_t                          GetHeight() const override { return m_height; }
    inline Ref<IRenderPass>                  GetRenderPass() const override { return m_renderPass; }
    inline const std::vector<Ref<ITexture>>& GetColorAttachments() const override { return m_colorAttachments; }
    inline Ref<ITexture>                     GetDepthAttachment() const override { return m_depthAttachment; }

    inline VkFramebuffer GetVkFrameBuffer() const { return m_frameBuffer; }

private:
    void Cleanup();
    void CreateVkFrameBuffer();

private:
    VkFramebuffer m_frameBuffer = VK_NULL_HANDLE;

    std::vector<Handle<VulkanTexture>> m_msaaTextures;

    uint32_t                   m_width;
    uint32_t                   m_height;
    Ref<IRenderPass>           m_renderPass = nullptr;
    std::vector<Ref<ITexture>> m_colorAttachments;
    Ref<ITexture>              m_depthAttachment = nullptr;
};

} // namespace Yogi
