#include "Layers/MaterialEditorLayer.h"
#include "Registry/AssetRegistry.h"
#include "Utils/ImGuiEnumCombo.h"
#include "Resources/ResourceManager/ResourceManager.h"

#include <imgui.h>

namespace Yogi
{

MaterialEditorLayer::MaterialEditorLayer() : Layer("Material Editor Layer") {}

void MaterialEditorLayer::OnUpdate(Timestep ts)
{
    ImGui::Begin("Material Editor");
    float cursorY = ImGui::GetCursorPosY();

    if (m_material)
    {
        ImGui::TextUnformatted(m_key.c_str());
        ImGui::Separator();

        ImGui::Text("Textures: %d", static_cast<int>(m_material->Textures.size()));
    }

    // blank space
    ImGui::SetCursorPosY(cursorY);
    ImVec2 viewportRegionMin = ImGui::GetWindowContentRegionMin();
    ImVec2 viewportRegionMax = ImGui::GetWindowContentRegionMax();
    ImGui::InvisibleButton("blank",
                           { std::max(viewportRegionMax.x - viewportRegionMin.x, 1.0f),
                             std::max(viewportRegionMax.y - viewportRegionMin.y, 1.0f) });
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ContentBrowserItem"))
        {
            const char*           path  = (const char*)payload->Data;
            std::filesystem::path fpath = path;
            if (fpath.extension().string() == ".mat")
            {
                m_key      = path;
                m_material = AssetManager::AcquireAsset<Material>(m_key);
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::End();
}

void MaterialEditorLayer::OnEvent(Event& event) {}

// -----------------------------------------------------------------------------------
bool MaterialEditorLayer::ImGuiMaterialData(const std::vector<std::string> shaderKeys, std::vector<uint8_t>& data)
{
    bool changed = false;

    return changed;
}

} // namespace Yogi
