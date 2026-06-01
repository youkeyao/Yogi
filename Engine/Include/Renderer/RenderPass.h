#pragma once

#include "Renderer/RHI/ICommandBuffer.h"
#include "Renderer/ShaderData.h"

namespace Yogi
{

class YG_API RenderPass
{
public:
    virtual ~RenderPass() = default;

    virtual void Initialize() = 0;
    virtual void Shutdown()   = 0;

    // Begin-of-frame reset. Default no-op so passes opt in. Intentionally
    // takes no slot parameter -- slot tracking is an internal detail of any
    // pool/arena that needs it (StagingArena uses cmd buffer fences directly,
    // ObjectCullPass swaps a private double-buffer index with no slot
    // semantics needed).
    virtual void BeginFrame() {}

    // Pass-specific contributions to the per-camera SceneFrame BDA table.
    // Default no-op so passes opt in.
    virtual void FillSceneFrame(SceneFrame& /*sceneFrame*/) {}
};

} // namespace Yogi
