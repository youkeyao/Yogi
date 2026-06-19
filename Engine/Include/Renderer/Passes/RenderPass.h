#pragma once

#include "Renderer/RHI/ICommandBuffer.h"
#include "Renderer/ShaderData.h"
#include "Renderer/PipelineRegistry.h"

namespace Yogi
{

// Base class for all render passes.
//
// Each concrete pass implements two virtual methods that let
// ForwardRenderSystem register the pass with PipelineRegistry
// without knowing the pass's shader paths or pipeline layout:
//
//   GetPassName()      – unique key used by PipelineRegistry
//   GetPipelineBuilder() – returns a builder lambda that knows how to
//                          construct (Early, Late) pipeline pairs for
//                          any material type.
//
// The builder captures pass-specific state (color format, etc.) that
// is set via SetTargetColorFormat() before the builder is retrieved.
class YG_API RenderPass
{
public:
    virtual ~RenderPass() = default;

    virtual void Initialize() = 0;
    virtual void Shutdown()   = 0;

    virtual void BeginFrame() {}

    virtual void FillSceneFrame(SceneFrame& sceneFrame) {}

    // Returns a pipeline builder that PipelineRegistry can use to
    // construct specialized (Early, Late) pipeline pairs for this pass.
    // The builder captures pass-specific state (color format, etc.).
    virtual SpecializedPipelineBuilder GetPipelineBuilder() = 0;

    // Pass name used as the key when registering / acquiring from
    // PipelineRegistry (e.g. "MeshletDraw", "Outline").
    virtual std::string GetPassName() const = 0;
};

} // namespace Yogi
