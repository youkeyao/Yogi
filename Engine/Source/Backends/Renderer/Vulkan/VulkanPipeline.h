#pragma once

#include "Renderer/RHI/IPipeline.h"
#include "VulkanDeviceContext.h"

namespace Yogi
{

class VulkanPipeline : public IPipeline
{
public:
    VulkanPipeline(const PipelineDesc& desc);
    virtual ~VulkanPipeline();

    inline PipelineType GetType() const override { return m_type; }

    inline VkPipeline       GetVkPipeline() const { return m_pipeline; }
    inline VkPipelineLayout GetVkPipelineLayout() const { return m_pipelineLayout; }

private:
    void CreateVkPipelineLayout(const PipelineDesc& desc);
    void CreateVkPipeline(const PipelineDesc& desc);

private:
    VkPipeline       m_pipeline       = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    PipelineType     m_type           = PipelineType::Graphics;
};

} // namespace Yogi
