#include "VulkanCommandBuffer.h"
#include "VulkanSwapChain.h"
#include "VulkanPipeline.h"
#include "VulkanBuffer.h"
#include "VulkanShaderResourceBinding.h"
#include "VulkanTexture.h"
#include "VulkanTextureView.h"
#include "VulkanUtils.h"

#include "Math/MathUtils.h"

#include <volk.h>

namespace Yogi
{

Owner<ICommandBuffer> ICommandBuffer::Create(const CommandBufferDesc& desc)
{
    return Owner<VulkanCommandBuffer>::Create(desc);
}

VulkanCommandBuffer::VulkanCommandBuffer(const CommandBufferDesc& desc) :
    m_commandBuffer(VK_NULL_HANDLE),
    m_usage(desc.Usage),
    m_queue(desc.Queue)
{
    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = context->GetVkCommandPoolForQueue(m_queue);
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    YG_CORE_ASSERT(vkAllocateCommandBuffers(context->GetVkDevice(), &allocInfo, &m_commandBuffer) == VK_SUCCESS,
                   "Vulkan: Failed to allocate command buffer!");

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    YG_CORE_ASSERT(vkCreateFence(context->GetVkDevice(), &fenceInfo, nullptr, &m_commandFence) == VK_SUCCESS,
                   "Vulkan: Failed to create fence!");
    vkResetFences(context->GetVkDevice(), 1, &m_commandFence);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    YG_CORE_ASSERT(vkCreateSemaphore(context->GetVkDevice(), &semaphoreInfo, nullptr, &m_signalSemaphore) == VK_SUCCESS,
                   "Vulkan: Failed to create signal semaphore!");
}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());
    if (m_commandBuffer != VK_NULL_HANDLE)
    {
        Wait();
        vkFreeCommandBuffers(context->GetVkDevice(), context->GetVkCommandPoolForQueue(m_queue), 1, &m_commandBuffer);
    }
    if (m_commandFence != VK_NULL_HANDLE)
    {
        vkDestroyFence(context->GetVkDevice(), m_commandFence, nullptr);
    }
    if (m_signalSemaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(context->GetVkDevice(), m_signalSemaphore, nullptr);
    }
}

void VulkanCommandBuffer::Begin()
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (m_usage == CommandBufferUsage::OneTimeSubmit)
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    YG_CORE_ASSERT(vkBeginCommandBuffer(m_commandBuffer, &beginInfo) == VK_SUCCESS,
                   "Vulkan: Failed to begin recording command buffer!");
}

void VulkanCommandBuffer::End()
{
    YG_CORE_ASSERT(vkEndCommandBuffer(m_commandBuffer) == VK_SUCCESS, "Vulkan: Failed to record command buffer!");
}

void VulkanCommandBuffer::Submit()
{
    YG_PROFILE_FUNCTION();

    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());

    VkCommandBufferSubmitInfo cmdSubmitInfo{};
    cmdSubmitInfo.sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cmdSubmitInfo.commandBuffer = m_commandBuffer;
    cmdSubmitInfo.deviceMask    = 0;

    VkSemaphoreSubmitInfo waitSemInfo{};
    if (m_waitSemaphore != VK_NULL_HANDLE)
    {
        waitSemInfo.sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        waitSemInfo.semaphore = m_waitSemaphore;
        waitSemInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        waitSemInfo.value     = 0;
    }
    VkSemaphoreSubmitInfo signalSemInfo{};
    if (m_signalSemaphore != VK_NULL_HANDLE)
    {
        signalSemInfo.sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        signalSemInfo.semaphore = m_signalSemaphore;
        signalSemInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        signalSemInfo.value     = 0;
    }

    VkSubmitInfo2 submitInfo{};
    submitInfo.sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfo.waitSemaphoreInfoCount   = m_waitSemaphore != VK_NULL_HANDLE ? 1u : 0u;
    submitInfo.pWaitSemaphoreInfos      = m_waitSemaphore != VK_NULL_HANDLE ? &waitSemInfo : nullptr;
    submitInfo.commandBufferInfoCount   = 1;
    submitInfo.pCommandBufferInfos      = &cmdSubmitInfo;
    submitInfo.signalSemaphoreInfoCount = m_signalSemaphore != VK_NULL_HANDLE ? 1u : 0u;
    submitInfo.pSignalSemaphoreInfos    = m_signalSemaphore != VK_NULL_HANDLE ? &signalSemInfo : nullptr;

    VkResult result = VK_NOT_READY;
    switch (m_queue)
    {
        case SubmitQueue::Graphics:
            result = vkQueueSubmit2(context->GetGraphicsQueue(), 1, &submitInfo, m_commandFence);
            break;
        case SubmitQueue::Compute:
            // Todo: To be implemented
            YG_CORE_ASSERT(false, "Compute queue submission not implemented yet!");
            break;
        case SubmitQueue::Transfer:
            result = vkQueueSubmit2(context->GetTransferQueue(), 1, &submitInfo, m_commandFence);
            break;
        default:
            YG_CORE_ASSERT(false, "Invalid SubmitQueue type!");
    }
    YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to submit render command buffer!");
    m_submitted = true;
}

void VulkanCommandBuffer::Wait()
{
    YG_PROFILE_FUNCTION();

    if (m_submitted)
    {
        VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());
        vkWaitForFences(context->GetVkDevice(), 1, &m_commandFence, VK_TRUE, UINT64_MAX);
        vkResetFences(context->GetVkDevice(), 1, &m_commandFence);
        m_submitted = false;
    }
}

// ---------- Helpers for BeginRendering / EndRendering ----------------------

namespace
{

VkAttachmentLoadOp ToVkLoadOp(LoadOp op)
{
    switch (op)
    {
        case LoadOp::Load:
            return VK_ATTACHMENT_LOAD_OP_LOAD;
        case LoadOp::Clear:
            return VK_ATTACHMENT_LOAD_OP_CLEAR;
        case LoadOp::DontCare:
            return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }
    return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
}

VkAttachmentStoreOp ToVkStoreOp(StoreOp op)
{
    switch (op)
    {
        case StoreOp::Store:
            return VK_ATTACHMENT_STORE_OP_STORE;
        case StoreOp::DontCare:
            return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }
    return VK_ATTACHMENT_STORE_OP_DONT_CARE;
}

} // anonymous namespace

void VulkanCommandBuffer::BeginRendering(const RenderingDesc& desc)
{
    std::vector<VkRenderingAttachmentInfo> colorInfos;
    colorInfos.reserve(desc.ColorAttachments.size());
    for (const auto& color : desc.ColorAttachments)
    {
        YG_CORE_ASSERT(color.View, "Vulkan: BeginRendering color attachment view is null");
        const VulkanTextureView* vkView = static_cast<const VulkanTextureView*>(color.View);
        YG_CORE_ASSERT(static_cast<const VulkanTexture*>(vkView->GetTexture())->GetCurrentLayout() ==
                           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                       "Vulkan: color attachment must be in COLOR_ATTACHMENT_OPTIMAL "
                       "before BeginRendering -- caller missed a Barrier()");

        VkRenderingAttachmentInfo info{};
        info.sType            = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        info.imageView        = vkView->GetVkImageView();
        info.imageLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        info.loadOp           = ToVkLoadOp(color.LoadAction);
        info.storeOp          = ToVkStoreOp(color.StoreAction);
        info.clearValue.color = {
            { color.ClearVal.Color[0], color.ClearVal.Color[1], color.ClearVal.Color[2], color.ClearVal.Color[3] }
        };

        if (color.ResolveView && desc.Samples > SampleCountFlagBits::Count1)
        {
            const VulkanTextureView* vkResolveView = static_cast<const VulkanTextureView*>(color.ResolveView);
            YG_CORE_ASSERT(static_cast<const VulkanTexture*>(vkResolveView->GetTexture())->GetCurrentLayout() ==
                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                           "Vulkan: resolve attachment must be in COLOR_ATTACHMENT_OPTIMAL before BeginRendering");
            info.resolveMode        = VK_RESOLVE_MODE_AVERAGE_BIT;
            info.resolveImageView   = vkResolveView->GetVkImageView();
            info.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
        colorInfos.push_back(info);
    }

    VkRenderingAttachmentInfo depthInfo{};
    bool                      hasDepth = false;
    if (desc.DepthAttachment.View)
    {
        const VulkanTextureView* vkView = static_cast<const VulkanTextureView*>(desc.DepthAttachment.View);
        YG_CORE_ASSERT(static_cast<const VulkanTexture*>(vkView->GetTexture())->GetCurrentLayout() ==
                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                       "Vulkan: depth attachment must be in DEPTH_STENCIL_ATTACHMENT_OPTIMAL "
                       "before BeginRendering -- caller missed a Barrier()");
        depthInfo.sType                           = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthInfo.imageView                       = vkView->GetVkImageView();
        depthInfo.imageLayout                     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthInfo.loadOp                          = ToVkLoadOp(desc.DepthAttachment.LoadAction);
        depthInfo.storeOp                         = ToVkStoreOp(desc.DepthAttachment.StoreAction);
        depthInfo.clearValue.depthStencil.depth   = desc.DepthAttachment.ClearVal.DepthStencil.Depth;
        depthInfo.clearValue.depthStencil.stencil = desc.DepthAttachment.ClearVal.DepthStencil.Stencil;
        hasDepth                                  = true;
    }

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset    = { 0, 0 };
    renderingInfo.renderArea.extent    = { desc.Width, desc.Height };
    renderingInfo.layerCount           = 1;
    renderingInfo.viewMask             = 0;
    renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorInfos.size());
    renderingInfo.pColorAttachments    = colorInfos.empty() ? nullptr : colorInfos.data();
    renderingInfo.pDepthAttachment     = hasDepth ? &depthInfo : nullptr;
    renderingInfo.pStencilAttachment   = nullptr;

    vkCmdBeginRendering(m_commandBuffer, &renderingInfo);
}

void VulkanCommandBuffer::EndRendering()
{
    vkCmdEndRendering(m_commandBuffer);
}

void VulkanCommandBuffer::SetPipeline(const IPipeline* pipeline)
{
    const VulkanPipeline& vkPipeline = *static_cast<const VulkanPipeline*>(pipeline);
    m_currentBindPoint =
        pipeline->GetType() == PipelineType::Compute ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS;
    m_currentLayout = vkPipeline.GetVkPipelineLayout();
    vkCmdBindPipeline(m_commandBuffer, m_currentBindPoint, vkPipeline.GetVkPipeline());
}

void VulkanCommandBuffer::SetVertexBuffer(const IBuffer* buffer, uint32_t offset)
{
    const VulkanBuffer& vkBuffer        = *static_cast<const VulkanBuffer*>(buffer);
    VkBuffer            vertexBuffers[] = { vkBuffer.GetVkBuffer() };
    VkDeviceSize        offsets[]       = { offset };
    vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, vertexBuffers, offsets);
}

void VulkanCommandBuffer::SetIndexBuffer(const IBuffer* buffer, uint32_t offset)
{
    const VulkanBuffer& vkBuffer = *static_cast<const VulkanBuffer*>(buffer);
    vkCmdBindIndexBuffer(m_commandBuffer, vkBuffer.GetVkBuffer(), offset, VK_INDEX_TYPE_UINT32);
}

void VulkanCommandBuffer::SetViewport(const Viewport& viewport)
{
    VkViewport vkViewport{};
    vkViewport.x        = viewport.X;
    vkViewport.y        = viewport.Y + viewport.Height;
    vkViewport.width    = viewport.Width;
    vkViewport.height   = -viewport.Height;
    vkViewport.minDepth = 0;
    vkViewport.maxDepth = 1;
    vkCmdSetViewport(m_commandBuffer, 0, 1, &vkViewport);
}

void VulkanCommandBuffer::SetScissor(const Scissor& scissor)
{
    VkRect2D vkScissor{};
    vkScissor.offset = { scissor.X, scissor.Y };
    vkScissor.extent = { scissor.Width, scissor.Height };
    vkCmdSetScissor(m_commandBuffer, 0, 1, &vkScissor);
}

void VulkanCommandBuffer::SetShaderResourceBinding(const IShaderResourceBinding* binding)
{
    YG_CORE_ASSERT(m_currentLayout != VK_NULL_HANDLE,
                   "Vulkan: SetShaderResourceBinding called before SetPipeline -- pipeline layout is unknown");
    const VulkanShaderResourceBinding& vkBinding = *static_cast<const VulkanShaderResourceBinding*>(binding);

    VkDescriptorSet set = vkBinding.GetVkDescriptorSet();
    vkCmdBindDescriptorSets(m_commandBuffer, m_currentBindPoint, m_currentLayout, 0, 1, &set, 0, nullptr);
}

void VulkanCommandBuffer::SetPushConstants(const IPipeline* pipeline,
                                           ShaderStage      stage,
                                           uint32_t         offset,
                                           uint32_t         size,
                                           const void*      data)
{
    const VulkanPipeline& vkPipeline = *static_cast<const VulkanPipeline*>(pipeline);
    vkCmdPushConstants(
        m_commandBuffer, vkPipeline.GetVkPipelineLayout(), YgShaderStage2VkShaderStage(stage), offset, size, data);
}

void VulkanCommandBuffer::Draw(uint32_t vertexCount,
                               uint32_t instanceCount,
                               uint32_t firstVertex,
                               uint32_t firstInstance)
{
    vkCmdDraw(m_commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanCommandBuffer::DrawIndexed(uint32_t indexCount,
                                      uint32_t instanceCount,
                                      uint32_t firstIndex,
                                      int32_t  vertexOffset,
                                      uint32_t firstInstance)
{
    vkCmdDrawIndexed(m_commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void VulkanCommandBuffer::DrawMeshTasks(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    vkCmdDrawMeshTasksEXT(m_commandBuffer, groupCountX, groupCountY, groupCountZ);
}

void VulkanCommandBuffer::DrawMeshTasksIndirect(const IBuffer* indirectBuffer,
                                                uint32_t       offset,
                                                uint32_t       drawCount,
                                                uint32_t       stride)
{
    const VulkanBuffer& vkIndirectBuffer = *static_cast<const VulkanBuffer*>(indirectBuffer);
    vkCmdDrawMeshTasksIndirectEXT(m_commandBuffer, vkIndirectBuffer.GetVkBuffer(), offset, drawCount, stride);
}

void VulkanCommandBuffer::DrawMeshTasksIndirectCount(const IBuffer* indirectBuffer,
                                                     uint32_t       indirectOffset,
                                                     const IBuffer* countBuffer,
                                                     uint32_t       countOffset,
                                                     uint32_t       maxDrawCount,
                                                     uint32_t       stride)
{
    const VulkanBuffer& vkIndirectBuffer = *static_cast<const VulkanBuffer*>(indirectBuffer);
    const VulkanBuffer& vkCountBuffer    = *static_cast<const VulkanBuffer*>(countBuffer);
    vkCmdDrawMeshTasksIndirectCountEXT(m_commandBuffer,
                                       vkIndirectBuffer.GetVkBuffer(),
                                       indirectOffset,
                                       vkCountBuffer.GetVkBuffer(),
                                       countOffset,
                                       maxDrawCount,
                                       stride);
}

void VulkanCommandBuffer::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    vkCmdDispatch(m_commandBuffer, groupCountX, groupCountY, groupCountZ);
}

void VulkanCommandBuffer::Barrier(const BarrierDesc& barrierDesc)
{
    if (barrierDesc.TextureView)
    {
        const VulkanTextureView* vkView = static_cast<const VulkanTextureView*>(barrierDesc.TextureView);
        const ITexture*          tex    = vkView->GetTexture();
        YG_CORE_ASSERT(tex, "Vulkan: Barrier called on a view whose texture is destroyed");
        const VulkanTexture* vkTex = static_cast<const VulkanTexture*>(tex);

        VkImageLayout oldLayout = (barrierDesc.BeforeState & ResourceState::Undefined) ? VK_IMAGE_LAYOUT_UNDEFINED :
                                                                                         vkTex->GetCurrentLayout();
        VkImageLayout newLayout = YgResourceState2VkImageLayout(barrierDesc.AfterState, tex->GetUsage());

        VkImageMemoryBarrier2 b{};
        b.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        b.srcStageMask        = YgResourceState2VkPipelineStage2(barrierDesc.BeforeState);
        b.srcAccessMask       = YgResourceState2VkAccess2(barrierDesc.BeforeState);
        b.dstStageMask        = YgResourceState2VkPipelineStage2(barrierDesc.AfterState);
        b.dstAccessMask       = YgResourceState2VkAccess2(barrierDesc.AfterState);
        b.oldLayout           = oldLayout;
        b.newLayout           = newLayout;
        b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.image               = vkTex->GetVkImage();
        b.subresourceRange    = vkView->GetVkSubresourceRange();

        VkDependencyInfo dep{};
        dep.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dep.imageMemoryBarrierCount = 1;
        dep.pImageMemoryBarriers    = &b;
        vkCmdPipelineBarrier2(m_commandBuffer, &dep);

        vkTex->SetCurrentLayout(newLayout);
        return;
    }
    else if (barrierDesc.Buffer)
    {
        const VulkanBuffer* vkBuffer = static_cast<const VulkanBuffer*>(barrierDesc.Buffer);

        VkBufferMemoryBarrier2 b{};
        b.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        b.srcStageMask        = YgResourceState2VkPipelineStage2(barrierDesc.BeforeState);
        b.srcAccessMask       = YgResourceState2VkAccess2(barrierDesc.BeforeState);
        b.dstStageMask        = YgResourceState2VkPipelineStage2(barrierDesc.AfterState);
        b.dstAccessMask       = YgResourceState2VkAccess2(barrierDesc.AfterState);
        b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.buffer              = vkBuffer->GetVkBuffer();
        b.offset              = barrierDesc.BufferOffset;
        b.size                = barrierDesc.BufferSize == 0 ? VK_WHOLE_SIZE : (VkDeviceSize)barrierDesc.BufferSize;

        VkDependencyInfo dep{};
        dep.sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dep.bufferMemoryBarrierCount = 1;
        dep.pBufferMemoryBarriers    = &b;
        vkCmdPipelineBarrier2(m_commandBuffer, &dep);
        return;
    }
    else
    {
        VkMemoryBarrier2 b{};
        b.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
        b.srcStageMask  = YgResourceState2VkPipelineStage2(barrierDesc.BeforeState);
        b.srcAccessMask = YgResourceState2VkAccess2(barrierDesc.BeforeState);
        b.dstStageMask  = YgResourceState2VkPipelineStage2(barrierDesc.AfterState);
        b.dstAccessMask = YgResourceState2VkAccess2(barrierDesc.AfterState);

        VkDependencyInfo dep{};
        dep.sType              = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dep.memoryBarrierCount = 1;
        dep.pMemoryBarriers    = &b;
        vkCmdPipelineBarrier2(m_commandBuffer, &dep);
        return;
    }
}

void VulkanCommandBuffer::Blit(const ITextureView* src, const ITextureView* dst, const BlitDesc& blitDesc)
{
    YG_CORE_ASSERT(src && dst, "Vulkan: Blit called with null view");

    const VulkanTextureView* vkSrcView = static_cast<const VulkanTextureView*>(src);
    const VulkanTextureView* vkDstView = static_cast<const VulkanTextureView*>(dst);

    const ITexture* srcTex = src->GetTexture();
    const ITexture* dstTex = dst->GetTexture();
    YG_CORE_ASSERT(srcTex && dstTex, "Vulkan: Blit called with view whose texture is destroyed");

    YG_CORE_ASSERT(src->GetMipLevelCount() == 1 && dst->GetMipLevelCount() == 1,
                   "Vulkan: Blit currently supports single-mip views only.");
    YG_CORE_ASSERT(src->GetArrayLayerCount() == dst->GetArrayLayerCount(),
                   "Vulkan: Blit src/dst layer counts must match.");
    YG_CORE_ASSERT(src->GetArrayLayerCount() == 1, "Vulkan: Blit currently supports single-layer views only.");

    const VulkanTexture& vkSrc = *static_cast<const VulkanTexture*>(srcTex);
    const VulkanTexture& vkDst = *static_cast<const VulkanTexture*>(dstTex);

    auto mipExtent = [](uint32_t width, uint32_t height, uint32_t mip) {
        return std::pair<uint32_t, uint32_t>{ MathUtils::Max(1u, width >> mip), MathUtils::Max(1u, height >> mip) };
    };

    const uint32_t srcMip = src->GetBaseMipLevel();
    const uint32_t dstMip = dst->GetBaseMipLevel();

    auto [srcMipWidth, srcMipHeight] = mipExtent(srcTex->GetWidth(), srcTex->GetHeight(), srcMip);
    auto [dstMipWidth, dstMipHeight] = mipExtent(dstTex->GetWidth(), dstTex->GetHeight(), dstMip);
    uint32_t srcWidth                = blitDesc.SrcWidth == 0 ? srcMipWidth : blitDesc.SrcWidth;
    uint32_t srcHeight               = blitDesc.SrcHeight == 0 ? srcMipHeight : blitDesc.SrcHeight;
    uint32_t dstWidth                = blitDesc.DstWidth == 0 ? dstMipWidth : blitDesc.DstWidth;
    uint32_t dstHeight               = blitDesc.DstHeight == 0 ? dstMipHeight : blitDesc.DstHeight;

    auto inferReadableState = [](const ITexture& texture) {
        if (texture.GetUsage() == ITexture::Usage::DepthStencil)
            return ResourceState::DepthRead;
        return ResourceState::FragmentShaderResource;
    };

    auto inferWritableState = [](const ITexture& texture) {
        if (texture.GetUsage() == ITexture::Usage::DepthStencil)
            return ResourceState::DepthWrite;
        if (texture.GetUsage() == ITexture::Usage::RenderTarget)
            return ResourceState::ColorAttachment;
        if (texture.GetUsage() == ITexture::Usage::Storage)
            return ResourceState::UnorderedAccess;
        return ResourceState::FragmentShaderResource;
    };

    ResourceState srcStableState = inferReadableState(*srcTex);
    ResourceState dstStableState = inferWritableState(*dstTex);

    VkImageAspectFlags srcAspect = vkSrcView->GetVkAspectMask();
    VkImageAspectFlags dstAspect = vkDstView->GetVkAspectMask();

    if (srcTex != dstTex)
    {
        Barrier(BarrierDesc{
            .TextureView = src,
            .BeforeState = srcStableState,
            .AfterState  = ResourceState::CopySource,
        });
        Barrier(BarrierDesc{
            .TextureView = dst,
            .BeforeState = ResourceState::Undefined,
            .AfterState  = ResourceState::CopyDestination,
        });

        VkImageBlit blit{};
        blit.srcSubresource.aspectMask     = srcAspect;
        blit.srcSubresource.mipLevel       = srcMip;
        blit.srcSubresource.baseArrayLayer = src->GetBaseArrayLayer();
        blit.srcSubresource.layerCount     = src->GetArrayLayerCount();
        blit.srcOffsets[0]                 = { 0, 0, 0 };
        blit.srcOffsets[1]                 = { (int32_t)srcWidth, (int32_t)srcHeight, 1 };
        blit.dstSubresource.aspectMask     = dstAspect;
        blit.dstSubresource.mipLevel       = dstMip;
        blit.dstSubresource.baseArrayLayer = dst->GetBaseArrayLayer();
        blit.dstSubresource.layerCount     = dst->GetArrayLayerCount();
        blit.dstOffsets[0]                 = { 0, 0, 0 };
        blit.dstOffsets[1]                 = { (int32_t)dstWidth, (int32_t)dstHeight, 1 };

        vkCmdBlitImage(m_commandBuffer,
                       vkSrc.GetVkImage(),
                       YgResourceState2VkImageLayout(ResourceState::CopySource, srcTex->GetUsage()),
                       vkDst.GetVkImage(),
                       YgResourceState2VkImageLayout(ResourceState::CopyDestination, dstTex->GetUsage()),
                       1,
                       &blit,
                       blitDesc.Filter == BlitFilter::Nearest || srcAspect == VK_IMAGE_ASPECT_DEPTH_BIT ?
                           VK_FILTER_NEAREST :
                           VK_FILTER_LINEAR);

        Barrier(BarrierDesc{
            .TextureView = src,
            .BeforeState = ResourceState::CopySource,
            .AfterState  = srcStableState,
        });
        Barrier(BarrierDesc{
            .TextureView = dst,
            .BeforeState = ResourceState::CopyDestination,
            .AfterState  = dstStableState,
        });
        return;
    }

    Barrier(BarrierDesc{
        .TextureView = dst,
        .BeforeState = dstStableState,
        .AfterState  = ResourceState::FragmentShaderResource,
    });
}

} // namespace Yogi
