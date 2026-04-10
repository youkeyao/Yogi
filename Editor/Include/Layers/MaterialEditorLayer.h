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
    bool ImGuiMaterialData(const std::vector<std::string> shaderKeys, std::vector<uint8_t>& data);

private:
    WRef<Material> m_material;
    std::string   m_key;
};

} // namespace Yogi