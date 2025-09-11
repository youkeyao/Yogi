#pragma once

#include "Core/Window.h"
#include "Renderer/RHI/ICommandBuffer.h"
#include "Renderer/RHI/ITexture.h"
#include "Renderer/RHI/IBuffer.h"
#include "Renderer/RHI/IPipeline.h"
#include "Renderer/RHI/IRenderPass.h"
#include "Renderer/RHI/ISwapChain.h"
#include "Renderer/RHI/IFrameBuffer.h"

namespace Yogi
{

class YG_API IDeviceContext
{
public:
    virtual ~IDeviceContext() = default;

    virtual void WaitIdle() = 0;

    static Handle<IDeviceContext> Create();
};

} // namespace Yogi
