#include "Layers/RenderPassEditorLayer.h"

#include "Registry/AssetRegistry.h"

#include <imgui.h>

namespace Yogi
{

RenderPassEditorLayer::RenderPassEditorLayer() : Layer("RenderPass Editor Layer") {}

void RenderPassEditorLayer::OnUpdate(Timestep ts)
{
    ImGui::Begin("RenderPass Editor");
    float cursorY = ImGui::GetCursorPosY();

    if (m_renderPass)
    {
        ImGui::Separator();
        
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
            if (fpath.extension().string() == ".rp")
            {
                m_key      = fpath.generic_string();
                m_renderPass = AssetManager::GetAsset<IRenderPass>(m_key);
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::End();
}

void RenderPassEditorLayer::OnEvent(Event& event) {}

} // namespace Yogi
