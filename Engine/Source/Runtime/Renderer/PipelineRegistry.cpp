#include "Renderer/PipelineRegistry.h"

namespace Yogi
{

void PipelineRegistry::RegisterPass(const PassKey& pass, SpecializedPipelineBuilder builder)
{
    m_builders[pass] = std::move(builder);
}

SpecializedPipelinePair PipelineRegistry::Acquire(const PassKey& pass, const std::string& shaderKey)
{
    if (shaderKey.empty())
    {
        YG_CORE_WARN("PipelineRegistry: Acquire called with empty shaderKey (pass='{0}')", pass);
        return {};
    }

    Key key{ pass, shaderKey };
    if (auto it = m_cache.find(key); it != m_cache.end())
        return it->second;

    auto bIt = m_builders.find(pass);
    if (bIt == m_builders.end())
    {
        YG_CORE_ERROR("PipelineRegistry: no builder registered for pass '{0}'", pass);
        return {};
    }

    // Builder now receives shaderKey directly; it can derive typeName internally
    SpecializedPipelinePair pair = bIt->second(pass, shaderKey);
    if (!pair.Early && !pair.Late)
    {
        YG_CORE_ERROR(
            "PipelineRegistry: builder for pass '{0}' returned an empty pair (shaderKey='{1}')", pass, shaderKey);
        return {};
    }

    m_cache[key] = pair;
    return pair;
}

} // namespace Yogi
