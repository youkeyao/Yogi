#pragma once

#include <Yogi.h>

namespace Yogi
{

class RenderPassEditorLayer : public Layer
{
public:
    RenderPassEditorLayer();
    virtual ~RenderPassEditorLayer() = default;

    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& event) override;

protected:
    bool ImGuiAttachment(AttachmentDesc& attachment);

private:
    WRef<IRenderPass> m_renderPass;
    std::string   m_key;
};

} // namespace Yogi