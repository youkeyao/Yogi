#pragma once

#include <Yogi.h>

namespace Yogi
{

class ImGuiBeginLayer : public Layer
{
public:
    ImGuiBeginLayer();
    virtual ~ImGuiBeginLayer() = default;

    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& event) override;

private:
    void SetThemeColor();

    void WindowInit(void* window);
    void WindowNewFrame();

    void RendererNewFrame();
};

} // namespace Yogi
