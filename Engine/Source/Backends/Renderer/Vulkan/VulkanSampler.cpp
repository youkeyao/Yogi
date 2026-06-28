#include "VulkanSampler.h"
#include "VulkanUtils.h"

#include <volk.h>

namespace Yogi
{

Owner<ISampler> ISampler::Create(const SamplerDesc& desc)
{
    return Owner<VulkanSampler>::Create(desc);
}

VulkanSampler::VulkanSampler(const SamplerDesc& desc)
{
    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());
    m_sampler                    = YgCreateVkSampler(context->GetVkDevice(), desc);
}

VulkanSampler::~VulkanSampler()
{
    if (m_sampler != VK_NULL_HANDLE)
    {
        VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());
        vkDestroySampler(context->GetVkDevice(), m_sampler, nullptr);
    }
}

} // namespace Yogi
