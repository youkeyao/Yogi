#pragma once

#include "Renderer/RHI/ICommandBuffer.h"
#include "VulkanFrameBuffer.h"
#include "VulkanRenderPass.h"

namespace Yogi
{

class VulkanCommandBuffer : public ICommandBuffer
{
public:
    VulkanCommandBuffer(const CommandBufferDesc& desc);
    virtual ~VulkanCommandBuffer();

    void Begin() override;
    void End() override;
    void Submit() override;
    void Wait() override;

    void BeginRenderPass(const Ref<IFrameBuffer>&       frameBuffer,
                         const std::vector<ClearValue>& colorClearValues,
                         const ClearValue&              depthClearValue) override;
    void EndRenderPass() override;

    void SetPipeline(const Ref<IPipeline>& pipeline) override;
    void SetVertexBuffer(const Ref<IBuffer>& buffer, uint32_t offset = 0) override;
    void SetIndexBuffer(const Ref<IBuffer>& buffer, uint32_t offset = 0) override;
    void SetViewport(const Viewport& viewport) override;
    void SetScissor(const Scissor& scissor) override;
    void SetShaderResourceBinding(const Ref<IShaderResourceBinding>& binding) override;

    void Draw(uint32_t vertexCount,
              uint32_t instanceCount = 1,
              uint32_t firstVertex   = 0,
              uint32_t firstInstance = 0) override;
    void DrawIndexed(uint32_t indexCount,
                     uint32_t instanceCount = 1,
                     uint32_t firstIndex    = 0,
                     int32_t  vertexOffset  = 0,
                     uint32_t firstInstance = 0) override;

    void TransitionImageLayout(VkImage image, ITexture::Usage usage, VkImageLayout oldLayout, VkImageLayout newLayout);

    inline VkCommandBuffer GetVkCommandBuffer() const { return m_commandBuffer; }

private:
    VkCommandBuffer    m_commandBuffer;
    CommandBufferUsage m_usage;
    SubmitQueue        m_queue;
};

} // namespace Yogi
