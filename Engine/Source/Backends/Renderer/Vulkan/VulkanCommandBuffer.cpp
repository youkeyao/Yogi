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
        throw std::runtime_error("Failed to allocate command buffer!");
    }
}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext().Get());
    if (m_commandBuffer != VK_NULL_HANDLE)
    {
        Wait();
        vkFreeCommandBuffers(context->GetVkDevice(), context->GetVkCommandPool(), 1, &m_commandBuffer);
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

    VkResult result = VK_NOT_READY;
    switch (m_queue)
    {
        case SubmitQueue::Graphics:
            result = vkQueueSubmit(context->GetGraphicsQueue(), 1, &submitInfo, swapChain->GetVkRenderCommandFence());
            break;
        case SubmitQueue::Compute:
            // Todo: To be implemented
            YG_CORE_ASSERT(false, "Compute queue submission not implemented yet!");
            break;
        case SubmitQueue::Transfer:
            result = vkQueueSubmit(context->GetTransferQueue(), 1, &submitInfo, VK_NULL_HANDLE);
            break;
        default:
            YG_CORE_ASSERT(false, "Invalid SubmitQueue type!");
    }
    YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to submit render command buffer!");
}

void VulkanCommandBuffer::Wait()
{
    YG_PROFILE_FUNCTION();

    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext().Get());

    switch (m_queue)
    {
        case SubmitQueue::Graphics:
            vkQueueWaitIdle(context->GetGraphicsQueue());
            break;
        case SubmitQueue::Compute:
            // To be implemented
            YG_CORE_ASSERT(false, "Compute queue submission not implemented yet!");
            break;
        case SubmitQueue::Transfer:
            vkQueueWaitIdle(context->GetTransferQueue());
            break;
        default:
            break;
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
