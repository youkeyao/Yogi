#pragma once

#include <Yogi.h>

namespace Yogi
{

class PipelineEditorLayer : public Layer
{
public:
    PipelineEditorLayer();
    virtual ~PipelineEditorLayer() = default;

    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& event) override;

private:
    Ref<IPipeline> m_pipeline;
    std::string   m_key;
};

} // namespace Yogi