#pragma once

#include "Renderer/RHI/ICommandBuffer.h"
#include "Renderer/RHI/IPipeline.h"
#include "Renderer/ShaderData.h"

namespace Yogi
{

class RenderGraph;
class RenderGraphBuilder;
class RenderGraphContext;

struct DrawBucket
{
    std::string ShaderKey;
    uint32_t    DrawBase;
    uint32_t    DrawCount;
    uint64_t    MaterialBufferAddr;
};

class YG_API RenderPass
{
public:
    RenderPass()          = default;
    virtual ~RenderPass() = default;

    RenderPass(const RenderPass&)            = delete;
    RenderPass& operator=(const RenderPass&) = delete;
    RenderPass(RenderPass&&)                 = delete;
    RenderPass& operator=(RenderPass&&)      = delete;

    // Initialization: called once when the pass is registered, before any frame.
    // Declare/allocate the named resources this pass produces here.
    virtual void Init(RenderGraph& graph) {}

    // Runtime: called every frame for all passes before any Execute runs.
    // Resolve consumed resources (GetBuffer/GetTexture), perform lazy rebuilds,
    // and declare this frame's resource-state transitions via the builder.
    virtual void Prepare(RenderGraphContext& context, RenderGraph& graph, RenderGraphBuilder& builder) {}

    virtual void Execute(RenderGraphContext& context) {}
};

} // namespace Yogi
