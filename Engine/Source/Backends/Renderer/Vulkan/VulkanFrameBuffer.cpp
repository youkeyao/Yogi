#include "VulkanFrameBuffer.h"
#include "VulkanTexture.h"

#include <volk.h>

namespace Yogi
{

Owner<IFrameBuffer> IFrameBuffer::Create(const FrameBufferDesc& desc)
{
    return Owner<VulkanFrameBuffer>::Create(desc);
}

VulkanFrameBuffer::VulkanFrameBuffer(const FrameBufferDesc& desc) :
    m_width(desc.Width),
    m_height(desc.Height),
    m_hasDepthAttachment(desc.DepthAttachment != nullptr)
{
    CreateVkFrameBuffer(desc);
}

VulkanFrameBuffer::~VulkanFrameBuffer()
{
    Cleanup();
}

// ----------------------------------------------------------------------------------------------

void VulkanFrameBuffer::Cleanup()
{
    if (m_frameBuffer != VK_NULL_HANDLE)
    {
        VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());
        vkDestroyFramebuffer(context->GetVkDevice(), m_frameBuffer, nullptr);
        m_frameBuffer = VK_NULL_HANDLE;
    }
    m_msaaTextures.clear();
}

void VulkanFrameBuffer::CreateVkFrameBuffer(const FrameBufferDesc& desc)
{
    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());
    Cleanup();

    SampleCountFlagBits      numSamples = desc.RenderPass->GetDesc().NumSamples;
    std::vector<VkImageView> attachments;
    for (int i = 0; i < desc.ColorAttachments.size(); ++i)
    {
        const VulkanTexture* texture = static_cast<const VulkanTexture*>(desc.ColorAttachments[i]);
        if (numSamples > SampleCountFlagBits::Count1)
        {
            TextureDesc msaaDesc{};
            msaaDesc.Width      = texture->GetWidth();
            msaaDesc.Height     = texture->GetHeight();
            msaaDesc.Format     = texture->GetFormat();
            msaaDesc.Usage      = ITexture::Usage::RenderTarget;
            msaaDesc.NumSamples = numSamples;

            m_msaaTextures.push_back(Owner<VulkanTexture>::Create(msaaDesc));
            attachments.push_back(m_msaaTextures.back()->GetVkImageView());
        }
        attachments.push_back(texture->GetVkImageView());
    }

    if (desc.DepthAttachment)
    {
        attachments.push_back(static_cast<const VulkanTexture*>(desc.DepthAttachment)->GetVkImageView());
    }

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass      = static_cast<const VulkanRenderPass*>(desc.RenderPass)->GetVkRenderPass();
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments    = attachments.data();
    framebufferInfo.width           = m_width;
    framebufferInfo.height          = m_height;
    framebufferInfo.layers          = 1;

    VkResult result = vkCreateFramebuffer(context->GetVkDevice(), &framebufferInfo, nullptr, &m_frameBuffer);
    YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to create framebuffer!");
}

} // namespace Yogi
