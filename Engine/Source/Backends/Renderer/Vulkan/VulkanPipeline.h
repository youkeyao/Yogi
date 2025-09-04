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

    inline VkPipeline GetVkPipeline() const { return m_pipeline; }

private:
    void CreateVkPipeline(const PipelineDesc& desc);

private:
    VkPipeline m_pipeline = VK_NULL_HANDLE;
};

} // namespace Yogi
