#pragma once

#include "Renderer/RHI/IQueryPool.h"
#include "VulkanDeviceContext.h"

namespace Yogi
{

class VulkanQueryPool : public IQueryPool
{
public:
    VulkanQueryPool(QueryType type, uint32_t count);
    virtual ~VulkanQueryPool();

    QueryType GetType() const override { return m_type; }
    uint32_t  GetCount() const override { return m_count; }

    bool   GetResults(uint32_t first, uint32_t count, uint64_t* out) const override;
    double GetTimestampPeriodNs() const override;

    inline VkQueryPool GetVkQueryPool() const { return m_queryPool; }

private:
    VkQueryPool m_queryPool = VK_NULL_HANDLE;
    QueryType   m_type      = QueryType::Timestamp;
    uint32_t    m_count     = 0;
};

} // namespace Yogi
