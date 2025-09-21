#pragma once

#include <Yogi.h>

namespace Yogi
{

class MaterialEditorLayer : public Layer
{
public:
    MaterialEditorLayer();
    virtual ~MaterialEditorLayer() = default;

    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& event) override;

private:
    Ref<Material> m_material;
    std::string   m_key;
};

} // namespace Yogi