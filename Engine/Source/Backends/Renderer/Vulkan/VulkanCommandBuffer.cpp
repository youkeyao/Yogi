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
    allocInfo.commandPool        = context->GetVkCommandPool();
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
        vkFreeCommandBuffers(context->GetVkDevice(), context->GetVkCommandPool(), 1, &m_commandBuffer);
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

    VulkanDeviceContext* context   = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());
    VulkanSwapChain*     swapChain = static_cast<VulkanSwapChain*>(Application::GetInstance().GetSwapChain());

    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &m_commandBuffer;

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    if (m_waitSemaphore != VK_NULL_HANDLE)
    {
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores    = &m_waitSemaphore;
        submitInfo.pWaitDstStageMask  = &waitStage;
    }
    if (m_signalSemaphore != VK_NULL_HANDLE)
    {
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = &m_signalSemaphore;
    }

    VkResult result = VK_NOT_READY;
    switch (m_queue)
    {
        case SubmitQueue::Graphics:
            result = vkQueueSubmit(context->GetGraphicsQueue(), 1, &submitInfo, m_commandFence);
            break;
        case SubmitQueue::Compute:
            // Todo: To be implemented
            YG_CORE_ASSERT(false, "Compute queue submission not implemented yet!");
            break;
        case SubmitQueue::Transfer:
            result = vkQueueSubmit(context->GetTransferQueue(), 1, &submitInfo, m_commandFence);
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

void VulkanCommandBuffer::BeginRenderPass(const IRenderPass*             renderPass,
                                          const IFrameBuffer*            frameBuffer,
                                          const std::vector<ClearValue>& colorClearValues,
                                          const ClearValue&              depthClearValue)
{
    const VulkanFrameBuffer& vkFrameBuffer = *static_cast<const VulkanFrameBuffer*>(frameBuffer);
    const VulkanRenderPass&  vkRenderPass  = *static_cast<const VulkanRenderPass*>(renderPass);
    SampleCountFlagBits      numSamples    = vkRenderPass.GetDesc().NumSamples;
    VkRenderPassBeginInfo    renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = vkRenderPass.GetVkRenderPass();
    renderPassInfo.framebuffer       = vkFrameBuffer.GetVkFrameBuffer();
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = { vkFrameBuffer.GetWidth(), vkFrameBuffer.GetHeight() };
    std::vector<VkClearValue> vkClearValues;
    for (size_t i = 0; i < colorClearValues.size(); ++i)
    {
        VkClearValue vkClearValue{ colorClearValues[i].Color[0],
                                   colorClearValues[i].Color[1],
                                   colorClearValues[i].Color[2],
                                   colorClearValues[i].Color[3] };
        vkClearValues.push_back(vkClearValue);
        if (numSamples > SampleCountFlagBits::Count1)
        {
            vkClearValues.push_back(vkClearValue);
        }
    }
    if (vkFrameBuffer.HasDepthAttachment())
    {
        vkClearValues.push_back(VkClearValue{
            depthClearValue.Color[0], depthClearValue.Color[1], depthClearValue.Color[2], depthClearValue.Color[3] });
    }
    renderPassInfo.clearValueCount = static_cast<uint32_t>(vkClearValues.size());
    renderPassInfo.pClearValues    = vkClearValues.data();
    vkCmdBeginRenderPass(m_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanCommandBuffer::EndRenderPass()
{
    vkCmdEndRenderPass(m_commandBuffer);
}

void VulkanCommandBuffer::SetPipeline(const IPipeline* pipeline)
{
    const VulkanPipeline& vkPipeline = *static_cast<const VulkanPipeline*>(pipeline);
    m_currentBindPoint =
        pipeline->GetType() == PipelineType::Compute ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS;
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
    const VulkanShaderResourceBinding& vkBinding = *static_cast<const VulkanShaderResourceBinding*>(binding);

    VkDescriptorSet set = vkBinding.GetVkDescriptorSet();
    vkCmdBindDescriptorSets(
        m_commandBuffer, m_currentBindPoint, vkBinding.GetVkPipelineLayout(), 0, 1, &set, 0, nullptr);
}

void VulkanCommandBuffer::SetPushConstants(const IShaderResourceBinding* binding,
                                           ShaderStage                   stage,
                                           uint32_t                      offset,
                                           uint32_t                      size,
                                           const void*                   data)
{
    const VulkanShaderResourceBinding& vkBinding = *static_cast<const VulkanShaderResourceBinding*>(binding);
    vkCmdPushConstants(
        m_commandBuffer, vkBinding.GetVkPipelineLayout(), YgShaderStage2VkShaderStage(stage), offset, size, data);
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

        VkImageLayout oldLayout = YgResourceState2VkImageLayout(barrierDesc.BeforeState, tex->GetUsage());
        VkImageLayout newLayout = YgResourceState2VkImageLayout(barrierDesc.AfterState, tex->GetUsage());

        VkImageMemoryBarrier barrier{};
        barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout           = oldLayout;
        barrier.newLayout           = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image               = vkTex->GetVkImage();
        barrier.srcAccessMask       = YgResourceState2VkAccess(barrierDesc.BeforeState);
        barrier.dstAccessMask       = YgResourceState2VkAccess(barrierDesc.AfterState);
        barrier.subresourceRange    = vkView->GetVkSubresourceRange();

        VkPipelineStageFlags sourceStage      = YgResourceState2VkPipelineStage(barrierDesc.BeforeState);
        VkPipelineStageFlags destinationStage = YgResourceState2VkPipelineStage(barrierDesc.AfterState);

        vkCmdPipelineBarrier(m_commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        return;
    }

    else if (barrierDesc.Buffer)
    {
        const VulkanBuffer* vkBuffer = static_cast<const VulkanBuffer*>(barrierDesc.Buffer);

        VkBufferMemoryBarrier barrier{};
        barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer              = vkBuffer->GetVkBuffer();
        barrier.offset              = barrierDesc.BufferOffset;
        barrier.size = barrierDesc.BufferSize == 0 ? VK_WHOLE_SIZE : static_cast<VkDeviceSize>(barrierDesc.BufferSize);
        barrier.srcAccessMask = YgResourceState2VkAccess(barrierDesc.BeforeState);
        barrier.dstAccessMask = YgResourceState2VkAccess(barrierDesc.AfterState);

        VkPipelineStageFlags sourceStage      = YgResourceState2VkPipelineStage(barrierDesc.BeforeState);
        VkPipelineStageFlags destinationStage = YgResourceState2VkPipelineStage(barrierDesc.AfterState);

        vkCmdPipelineBarrier(m_commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 1, &barrier, 0, nullptr);
        return;
    }

    else
    {
        VkMemoryBarrier barrier{};
        barrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = YgResourceState2VkAccess(barrierDesc.BeforeState);
        barrier.dstAccessMask = YgResourceState2VkAccess(barrierDesc.AfterState);

        VkPipelineStageFlags sourceStage      = YgResourceState2VkPipelineStage(barrierDesc.BeforeState);
        VkPipelineStageFlags destinationStage = YgResourceState2VkPipelineStage(barrierDesc.AfterState);

        vkCmdPipelineBarrier(m_commandBuffer, sourceStage, destinationStage, 0, 1, &barrier, 0, nullptr, 0, nullptr);
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
            return ResourceState::Present;
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
        // Destination is fully overwritten by the blit, so transitioning from UNDEFINED is valid.
        Barrier(BarrierDesc{
            .TextureView = dst,
            .BeforeState = ResourceState::None,
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
