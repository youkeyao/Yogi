#pragma once

#include "Renderer/RHI/ISampler.h"
#include "VulkanDeviceContext.h"

namespace Yogi
{

class VulkanSampler : public ISampler
{
public:
    VulkanSampler(const SamplerDesc& desc);
    virtual ~VulkanSampler();

    inline VkSampler GetVkSampler() const { return m_sampler; }

private:
    VkSampler m_sampler = VK_NULL_HANDLE;
};

} // namespace Yogi
