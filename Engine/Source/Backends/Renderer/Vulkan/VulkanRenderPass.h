#pragma once

#include "Renderer/RHI/IRenderPass.h"
#include "VulkanDeviceContext.h"

namespace Yogi
{

class VulkanRenderPass : public IRenderPass
{
public:
    VulkanRenderPass(const RenderPassDesc& desc);
    virtual ~VulkanRenderPass();

    inline VkRenderPass GetVkRenderPass() const { return m_RenderPass; }

private:
    void CreateVkRenderPass();

private:
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
};

} // namespace Yogi
