#pragma once

#include "Renderer/RHI/IRenderPass.h"
#include "VulkanDeviceContext.h"

namespace Yogi
{

class VulkanRenderPass : public IRenderPass
{
public:
    VulkanRenderPass(const RenderPassDesc& desc);
    virtual ~VulkanRenderPass();

    const std::vector<AttachmentDesc>& GetColorAttachments() const override { return m_colorAttachments; }
    AttachmentDesc                     GetDepthAttachment() const override { return m_depthAttachment; }
    SampleCountFlagBits                GetNumSamples() const override { return m_numSamples; }

    inline VkRenderPass GetVkRenderPass() const { return m_RenderPass; }

private:
    void CreateVkRenderPass();

private:
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;

    std::vector<AttachmentDesc> m_colorAttachments;
    AttachmentDesc              m_depthAttachment;
    SampleCountFlagBits         m_numSamples;
};

} // namespace Yogi
