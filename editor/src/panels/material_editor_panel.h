#pragma once

#include <engine.h>

namespace Yogi {

class MaterialEditorPanel
{
public:
    MaterialEditorPanel();
    ~MaterialEditorPanel() = default;

    void on_imgui_render();

private:
    Ref<Material> m_material;
    std::string   m_parent_path;
};

}  // namespace Yogi