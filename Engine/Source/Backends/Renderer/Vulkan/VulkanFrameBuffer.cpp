#include "VulkanFrameBuffer.h"
#include "VulkanTexture.h"
#include "VulkanTextureView.h"

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
    m_msaaViews.clear();
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
        const ITextureView*      view   = desc.ColorAttachments[i];
        const VulkanTextureView* vkView = static_cast<const VulkanTextureView*>(view);
        const ITexture*          tex    = view->GetTexture();
        YG_CORE_ASSERT(tex, "VulkanFrameBuffer: color attachment view's texture is destroyed");

        if (numSamples > SampleCountFlagBits::Count1)
        {
            TextureDesc msaaDesc{};
            msaaDesc.Width      = tex->GetWidth();
            msaaDesc.Height     = tex->GetHeight();
            msaaDesc.Format     = tex->GetFormat();
            msaaDesc.Usage      = ITexture::Usage::RenderTarget;
            msaaDesc.NumSamples = numSamples;

            Owner<ITexture>     msaaTex  = ITexture::Create(msaaDesc);
            Owner<ITextureView> msaaView = ITextureView::Create(WRef<ITexture>::Create(msaaTex), {});

            const VulkanTextureView* vkMsaaView = static_cast<const VulkanTextureView*>(msaaView.Get());
            attachments.push_back(vkMsaaView->GetVkImageView());

            m_msaaTextures.push_back(std::move(msaaTex));
            m_msaaViews.push_back(std::move(msaaView));
        }
        attachments.push_back(vkView->GetVkImageView());
    }

    if (desc.DepthAttachment)
    {
        const VulkanTextureView* vkDepthView = static_cast<const VulkanTextureView*>(desc.DepthAttachment);
        attachments.push_back(vkDepthView->GetVkImageView());
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
