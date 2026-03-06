#include "VulkanCommandBuffer.h"
#include "VulkanSwapChain.h"
#include "VulkanPipeline.h"
#include "VulkanBuffer.h"
#include "VulkanShaderResourceBinding.h"

#include <volk.h>

namespace Yogi
{

Handle<ICommandBuffer> ICommandBuffer::Create(const CommandBufferDesc& desc)
{
    return Handle<VulkanCommandBuffer>::Create(desc);
}

VulkanCommandBuffer::VulkanCommandBuffer(const CommandBufferDesc& desc) :
    m_commandBuffer(VK_NULL_HANDLE),
    m_usage(desc.Usage),
    m_queue(desc.Queue)
{
    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext().Get());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = context->GetVkCommandPool();
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(context->GetVkDevice(), &allocInfo, &m_commandBuffer) != VK_SUCCESS)
    {
        YG_CORE_ERROR("Vulkan: Failed to allocate command buffer!");
    }

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
    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext().Get());
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

    VulkanDeviceContext* context   = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext().Get());
    VulkanSwapChain*     swapChain = static_cast<VulkanSwapChain*>(Application::GetInstance().GetSwapChain().Get());

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
        VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext().Get());
        vkWaitForFences(context->GetVkDevice(), 1, &m_commandFence, VK_TRUE, UINT64_MAX);
        vkResetFences(context->GetVkDevice(), 1, &m_commandFence);
        m_submitted = false;
    }
}

void VulkanCommandBuffer::BeginRenderPass(const Ref<IFrameBuffer>&       frameBuffer,
                                          const std::vector<ClearValue>& colorClearValues,
                                          const ClearValue&              depthClearValue)
{
    Ref<VulkanFrameBuffer> vkFrameBuffer = Ref<VulkanFrameBuffer>::Cast(frameBuffer);
    Ref<VulkanRenderPass>  vkRenderPass  = Ref<VulkanRenderPass>::Cast(vkFrameBuffer->GetRenderPass());
    SampleCountFlagBits    numSamples    = vkRenderPass->GetDesc().NumSamples;
    VkRenderPassBeginInfo  renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = vkRenderPass->GetVkRenderPass();
    renderPassInfo.framebuffer       = vkFrameBuffer->GetVkFrameBuffer();
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = { vkFrameBuffer->GetWidth(), vkFrameBuffer->GetHeight() };
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
    if (vkFrameBuffer->GetDepthAttachment())
    {
        vkClearValues.push_back(VkClearValue{
            depthClearValue.Color[0], depthClearValue.Color[1], depthClearValue.Color[2], depthClearValue.Color[3] });
    }
    renderPassInfo.clearValueCount = static_cast<uint32_t>(vkClearValues.size());
    renderPassInfo.pClearValues    = vkClearValues.data();
    vkCmdBeginRenderPass(m_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanCommandBuffer::EndRenderPass() { vkCmdEndRenderPass(m_commandBuffer); }

void VulkanCommandBuffer::SetPipeline(const Ref<IPipeline>& pipeline)
{
    Ref<VulkanPipeline> vkPipeline = Ref<VulkanPipeline>::Cast(pipeline);
    vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline->GetVkPipeline());
}

void VulkanCommandBuffer::SetVertexBuffer(const Ref<IBuffer>& buffer, uint32_t offset)
{
    Ref<VulkanBuffer> vkBuffer        = Ref<VulkanBuffer>::Cast(buffer);
    VkBuffer          vertexBuffers[] = { vkBuffer->GetVkBuffer() };
    VkDeviceSize      offsets[]       = { offset };
    vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, vertexBuffers, offsets);
}

void VulkanCommandBuffer::SetIndexBuffer(const Ref<IBuffer>& buffer, uint32_t offset)
{
    Ref<VulkanBuffer> vkBuffer = Ref<VulkanBuffer>::Cast(buffer);
    vkCmdBindIndexBuffer(m_commandBuffer, vkBuffer->GetVkBuffer(), offset, VK_INDEX_TYPE_UINT32);
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

void VulkanCommandBuffer::SetShaderResourceBinding(const Ref<IShaderResourceBinding>& binding)
{
    Ref<VulkanShaderResourceBinding> vkBinding = Ref<VulkanShaderResourceBinding>::Cast(binding);

    VkDescriptorSet set = vkBinding->GetVkDescriptorSet();
    vkCmdBindDescriptorSets(
        m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkBinding->GetVkPipelineLayout(), 0, 1, &set, 0, nullptr);
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

void VulkanCommandBuffer::Blit(const Ref<ITexture>& src, const Ref<ITexture>& dst)
{
    Ref<VulkanTexture> vkSrc = Ref<VulkanTexture>::Cast(src);
    Ref<VulkanTexture> vkDst = Ref<VulkanTexture>::Cast(dst);
    if (src != dst)
    {
        TransitionImageLayout(
            vkSrc->GetVkImage(), vkSrc->GetUsage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        TransitionImageLayout(
            vkDst->GetVkImage(), vkDst->GetUsage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkImageBlit blit{};
        blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel       = 0;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount     = 1;
        blit.srcOffsets[0]                 = { 0, 0, 0 };
        blit.srcOffsets[1]                 = { (int32_t)vkSrc->GetWidth(), (int32_t)vkSrc->GetHeight(), 1 };
        blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel       = 0;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount     = 1;
        blit.dstOffsets[0]                 = { 0, 0, 0 };
        blit.dstOffsets[1]                 = { (int32_t)vkDst->GetWidth(), (int32_t)vkDst->GetHeight(), 1 };

        vkCmdBlitImage(m_commandBuffer,
                       vkSrc->GetVkImage(),
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       vkDst->GetVkImage(),
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1,
                       &blit,
                       VK_FILTER_LINEAR);
    }

    TransitionImageLayout(
        vkDst->GetVkImage(), vkDst->GetUsage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void VulkanCommandBuffer::TransitionImageLayout(VkImage         image,
                                                ITexture::Usage usage,
                                                VkImageLayout   oldLayout,
                                                VkImageLayout   newLayout)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = oldLayout;
    barrier.newLayout                       = newLayout;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = image;
    barrier.subresourceRange.aspectMask     = usage != ITexture::Usage::DepthStencil ?
            VK_IMAGE_ASPECT_COLOR_BIT :
            VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;

    barrier.srcAccessMask                 = AccessMaskFromImageLayout(oldLayout, false);
    barrier.dstAccessMask                 = AccessMaskFromImageLayout(newLayout, true);
    VkPipelineStageFlags sourceStage      = PipelineStageFromImageLayout(oldLayout, false);
    VkPipelineStageFlags destinationStage = PipelineStageFromImageLayout(newLayout, true);

    vkCmdPipelineBarrier(m_commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

} // namespace Yogi
