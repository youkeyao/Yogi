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
    Ref<ICommandBuffer> m_commandBuffer;
    Ref<IRenderPass>    m_renderPass;

    std::unordered_map<uint64_t, Ref<IFrameBuffer>> m_frameBuffers;
};

} // namespace Yogi
