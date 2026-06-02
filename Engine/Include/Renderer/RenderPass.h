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

    virtual void BeginFrame() {}

    virtual void FillSceneFrame(SceneFrame& sceneFrame) {}
};

} // namespace Yogi
