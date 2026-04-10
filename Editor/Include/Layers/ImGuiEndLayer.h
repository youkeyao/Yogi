#pragma once

#include <Yogi.h>

namespace Yogi
{

class ImGuiEndLayer : public Layer
{
public:
    ImGuiEndLayer();
    virtual ~ImGuiEndLayer();

    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& event) override;

private:
    bool OnWindowResize(WindowResizeEvent& e);
    void WindowShutdown();

    void RendererInit();
    void RendererDraw();
    void RendererShutdown();

private:
    WRef<ICommandBuffer> m_commandBuffer;
    WRef<IRenderPass>    m_renderPass;

    std::unordered_map<uint64_t, WRef<IFrameBuffer>> m_frameBuffers;
};

} // namespace Yogi
