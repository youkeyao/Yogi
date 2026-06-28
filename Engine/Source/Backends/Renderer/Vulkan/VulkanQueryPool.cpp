#include "VulkanQueryPool.h"

#include <volk.h>

namespace Yogi
{

Owner<IQueryPool> IQueryPool::Create(QueryType type, uint32_t count)
{
    return Owner<VulkanQueryPool>::Create(type, count);
}

VulkanQueryPool::VulkanQueryPool(QueryType type, uint32_t count) : m_type(type), m_count(count)
{
    YG_CORE_ASSERT(count > 0, "Vulkan: QueryPool count must be > 0");

    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());

    VkQueryPoolCreateInfo info{};
    info.sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    info.queryCount = count;
    switch (type)
    {
        case QueryType::Timestamp:
            info.queryType = VK_QUERY_TYPE_TIMESTAMP;
            break;
        case QueryType::Occlusion:
            info.queryType = VK_QUERY_TYPE_OCCLUSION;
            break;
        default:
            YG_CORE_ASSERT(false, "Vulkan: Unsupported QueryType");
            info.queryType = VK_QUERY_TYPE_TIMESTAMP;
            break;
    }

    VkResult result = vkCreateQueryPool(context->GetVkDevice(), &info, nullptr, &m_queryPool);
    YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to create query pool!");
}

VulkanQueryPool::~VulkanQueryPool()
{
    if (m_queryPool != VK_NULL_HANDLE)
    {
        VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());
        vkDestroyQueryPool(context->GetVkDevice(), m_queryPool, nullptr);
    }
}

bool VulkanQueryPool::GetResults(uint32_t first, uint32_t count, uint64_t* out) const
{
    YG_CORE_ASSERT(out && first + count <= m_count, "Vulkan: QueryPool::GetResults out of range");

    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());
    VkResult             result  = vkGetQueryPoolResults(context->GetVkDevice(),
                                                         m_queryPool,
                                                         first,
                                                         count,
                                                         static_cast<size_t>(count) * sizeof(uint64_t),
                                                         out,
                                                         sizeof(uint64_t),
                                                         VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
    return result == VK_SUCCESS;
}

double VulkanQueryPool::GetTimestampPeriodNs() const
{
    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());
    return static_cast<double>(context->GetVkPhysicalDeviceProperties().limits.timestampPeriod);
}

} // namespace Yogi
