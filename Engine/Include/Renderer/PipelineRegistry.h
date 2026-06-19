#pragma once

#include "Renderer/RHI/IPipeline.h"

namespace Yogi
{

using PassKey = std::string;

struct SpecializedPipelinePair
{
    WRef<IPipeline> Early;
    WRef<IPipeline> Late;
};

// Builder receives shaderKey; it can derive typeName internally via Slang reflection
using SpecializedPipelineBuilder =
    std::function<SpecializedPipelinePair(const PassKey& pass, const std::string& shaderKey)>;

class YG_API PipelineRegistry
{
public:
    PipelineRegistry()  = default;
    ~PipelineRegistry() = default;

    void RegisterPass(const PassKey& pass, SpecializedPipelineBuilder builder);

    SpecializedPipelinePair Acquire(const PassKey& pass, const std::string& shaderKey);

private:
    struct Key
    {
        PassKey     Pass;
        std::string ShaderKey;
        bool        operator==(const Key& other) const { return Pass == other.Pass && ShaderKey == other.ShaderKey; }
    };
    struct KeyHash
    {
        size_t operator()(const Key& k) const noexcept
        {
            return std::hash<std::string>{}(k.Pass) ^ std::hash<std::string>{}(k.ShaderKey);
        }
    };

    std::unordered_map<PassKey, SpecializedPipelineBuilder, std::hash<std::string>> m_builders;
    std::unordered_map<Key, SpecializedPipelinePair, KeyHash>                       m_cache;
};

} // namespace Yogi
