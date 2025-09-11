#include "VulkanRenderPass.h"
#include "Renderer/RHI/IRenderPass.h"
#include "VulkanDeviceContext.h"

namespace Yogi
{

Handle<IRenderPass> IRenderPass::Create(const RenderPassDesc& desc) { return Handle<VulkanRenderPass>::Create(desc); }

VulkanRenderPass::VulkanRenderPass(const RenderPassDesc& desc) :
    m_colorAttachments(desc.ColorAttachments),
    m_depthAttachment(desc.DepthAttachment),
    m_numSamples(desc.NumSamples)
{
    CreateVkRenderPass();
}

VulkanRenderPass::~VulkanRenderPass()
{
    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext().Get());

    if (m_RenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(context->GetVkDevice(), m_RenderPass, nullptr);
        m_RenderPass = VK_NULL_HANDLE;
    }
}

// ----------------------------------------------------------------------------------------------

void VulkanRenderPass::CreateVkRenderPass()
{
    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext().Get());

    std::vector<VkAttachmentDescription> allAttachments;
    std::vector<VkAttachmentReference>   colorAttachmentRefs;
    std::vector<VkAttachmentReference>   resolveAttachmentRefs;
    for (auto& attachment : m_colorAttachments)
    {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format  = YgTextureFormat2VkFormat(attachment.Format);
        colorAttachment.samples = (VkSampleCountFlagBits)m_numSamples;
        colorAttachment.loadOp  = attachment.ColorLoadOp == LoadOp::Clear ? VK_ATTACHMENT_LOAD_OP_CLEAR :
             attachment.ColorLoadOp == LoadOp::Load                       ? VK_ATTACHMENT_LOAD_OP_LOAD :
                                                                            VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.storeOp =
            attachment.ColorStoreOp == StoreOp::Store ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout    = AttachmentUsage2VkImageLayout(attachment.Usage);

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = static_cast<uint32_t>(allAttachments.size());
        colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        allAttachments.push_back(colorAttachment);
        colorAttachmentRefs.push_back(colorAttachmentRef);

        if (m_numSamples > SampleCountFlagBits::Count1)
        {
            colorAttachment.samples       = VK_SAMPLE_COUNT_1_BIT;
            colorAttachmentRef.attachment = static_cast<uint32_t>(allAttachments.size());

            allAttachments.push_back(colorAttachment);
            resolveAttachmentRefs.push_back(colorAttachmentRef);
        }
    }
    VkAttachmentDescription depthAttachment{};
    VkAttachmentReference   depthAttachmentRef{};
    VkSubpassDescription    subpass{};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = static_cast<uint32_t>(colorAttachmentRefs.size());
    subpass.pColorAttachments       = colorAttachmentRefs.data();
    subpass.pResolveAttachments     = resolveAttachmentRefs.data();
    subpass.pDepthStencilAttachment = nullptr;
    if (m_depthAttachment.Format != ITexture::Format::None)
    {
        depthAttachment.format  = YgTextureFormat2VkFormat(m_depthAttachment.Format);
        depthAttachment.samples = (VkSampleCountFlagBits)m_numSamples;
        depthAttachment.loadOp  = m_depthAttachment.ColorLoadOp == LoadOp::Clear ? VK_ATTACHMENT_LOAD_OP_CLEAR :
             m_depthAttachment.ColorLoadOp == LoadOp::Load                       ? VK_ATTACHMENT_LOAD_OP_LOAD :
                                                                                   VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.storeOp = m_depthAttachment.ColorStoreOp == StoreOp::Store ? VK_ATTACHMENT_STORE_OP_STORE :
                                                                                     VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp   = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp  = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout   = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachmentRef.attachment   = static_cast<uint32_t>(allAttachments.size());
        depthAttachmentRef.layout       = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
        allAttachments.push_back(depthAttachment);
    }
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(allAttachments.size());
    renderPassInfo.pAttachments    = allAttachments.data();
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies   = &dependency;

    VkResult result = vkCreateRenderPass(context->GetVkDevice(), &renderPassInfo, nullptr, &m_RenderPass);
    YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to create render pass!");
}

} // namespace Yogi
