#pragma once

namespace Yogi
{

enum class QueryType : uint8_t
{
    Timestamp = 0,
    Occlusion,
};

class YG_API IQueryPool
{
public:
    virtual ~IQueryPool() = default;

    virtual QueryType GetType() const  = 0;
    virtual uint32_t  GetCount() const = 0;

    virtual bool GetResults(uint32_t first, uint32_t count, uint64_t* out) const = 0;

    virtual double GetTimestampPeriodNs() const = 0;

    static Owner<IQueryPool> Create(QueryType type, uint32_t count);
};

template <>
template <typename... Args>
inline Owner<IQueryPool> Owner<IQueryPool>::Create(Args&&... args)
{
    return IQueryPool::Create(std::forward<Args>(args)...);
}

} // namespace Yogi
