#include "VulkanFrameBuffer.h"
#include "VulkanTexture.h"

#include <volk.h>

namespace Yogi
{

Handle<IFrameBuffer> IFrameBuffer::Create(const FrameBufferDesc& desc)
{
    return Handle<VulkanFrameBuffer>::Create(desc);
}

VulkanFrameBuffer::VulkanFrameBuffer(const FrameBufferDesc& desc) :
    m_width(desc.Width),
    m_height(desc.Height),
    m_renderPass(desc.RenderPass),
    m_colorAttachments(desc.ColorAttachments),
    m_depthAttachment(desc.DepthAttachment)
{
    CreateVkFrameBuffer();
}

VulkanFrameBuffer::~VulkanFrameBuffer() { Cleanup(); }

// ----------------------------------------------------------------------------------------------

void VulkanFrameBuffer::Cleanup()
{
    if (m_frameBuffer != VK_NULL_HANDLE)
    {
        VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext().Get());
        vkDestroyFramebuffer(context->GetVkDevice(), m_frameBuffer, nullptr);
        m_frameBuffer = VK_NULL_HANDLE;
    }
    m_msaaTextures.clear();
}

void VulkanFrameBuffer::CreateVkFrameBuffer()
{
    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext().Get());
    Cleanup();

    SampleCountFlagBits      numSamples = m_renderPass->GetNumSamples();
    std::vector<VkImageView> attachments;
    for (int i = 0; i < m_colorAttachments.size(); ++i)
    {
        Ref<VulkanTexture> texture = Ref<VulkanTexture>::Cast(m_colorAttachments[i]);
        if (numSamples > SampleCountFlagBits::Count1)
        {
            TextureDesc msaaDesc{};
            msaaDesc.Width      = texture->GetWidth();
            msaaDesc.Height     = texture->GetHeight();
            msaaDesc.Format     = texture->GetFormat();
            msaaDesc.Usage      = ITexture::Usage::RenderTarget;
            msaaDesc.NumSamples = numSamples;

            m_msaaTextures.push_back(Handle<VulkanTexture>::Create(msaaDesc));
            attachments.push_back(m_msaaTextures.back()->GetVkImageView());
        }
        attachments.push_back(texture->GetVkImageView());
    }

    if (m_depthAttachment)
    {
        attachments.push_back(Ref<VulkanTexture>::Cast(m_depthAttachment)->GetVkImageView());
    }

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass      = Ref<VulkanRenderPass>::Cast(m_renderPass)->GetVkRenderPass();
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments    = attachments.data();
    framebufferInfo.width           = m_width;
    framebufferInfo.height          = m_height;
    framebufferInfo.layers          = 1;

    VkResult result = vkCreateFramebuffer(context->GetVkDevice(), &framebufferInfo, nullptr, &m_frameBuffer);
    YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to create framebuffer!");
}

} // namespace Yogi
