
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

    inline uint32_t GetWidth() const override { return m_width; }
    inline uint32_t GetHeight() const override { return m_height; }

    inline VkFramebuffer GetVkFrameBuffer() const { return m_frameBuffer; }
    inline bool          HasDepthAttachment() const { return m_hasDepthAttachment; }

private:
    void Cleanup();
    void CreateVkFrameBuffer(const FrameBufferDesc& desc);

private:
    VkFramebuffer m_frameBuffer = VK_NULL_HANDLE;

    std::vector<Owner<VulkanTexture>> m_msaaTextures;

    uint32_t m_width;
    uint32_t m_height;
    bool     m_hasDepthAttachment = false;
};

} // namespace Yogi
