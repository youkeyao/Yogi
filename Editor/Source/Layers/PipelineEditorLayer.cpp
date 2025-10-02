#include "Layers/PipelineEditorLayer.h"
#include "Resources/AssetManager/AssetManager.h"
#include "Registry/AssetRegistry.h"
#include "Utils/ImGuiEnumCombo.h"

#include <imgui.h>

namespace Yogi
{

PipelineEditorLayer::PipelineEditorLayer() : Layer("Pipeline Editor Layer") {}

void PipelineEditorLayer::OnUpdate(Timestep ts)
{
    ImGui::Begin("Pipeline Editor");
    float cursorY = ImGui::GetCursorPosY();

    if (m_pipeline)
    {
        ImGui::TextUnformatted(m_key.c_str());
        ImGui::Separator();
        auto pipelineDesc = m_pipeline->GetDesc();
        bool changed      = false;
        if (ImGui::TreeNode("Shaders"))
        {
            for (auto& shader : pipelineDesc.Shaders)
            {
                std::string shaderKey = AssetManager::GetAssetKey(shader);
                if (ImGui::BeginCombo(("##" + shaderKey).c_str(), shaderKey.c_str()))
                {
                    for (auto& key : AssetRegistry::GetKeys<ShaderDesc>())
                    {
                        if (ImGui::Selectable(key.c_str(), key == shaderKey))
                        {
                            shader = AssetManager::GetAsset<ShaderDesc>(key);
                            changed |= true;
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            if (ImGui::Button("Add Shader"))
            {
                pipelineDesc.Shaders.push_back(
                    AssetManager::GetAsset<ShaderDesc>(AssetRegistry::GetKeys<ShaderDesc>().front()));
                changed |= true;
            }
            ImGui::TreePop();
        }

        std::string renderPassKey = AssetManager::GetAssetKey(pipelineDesc.RenderPass);
        if (ImGui::BeginCombo("Render Pass", renderPassKey.c_str()))
        {
            for (auto& key : AssetRegistry::GetKeys<IRenderPass>())
            {
                if (ImGui::Selectable(key.c_str(), key == renderPassKey))
                {
                    pipelineDesc.RenderPass = AssetManager::GetAsset<IRenderPass>(key);
                    changed |= true;
                }
            }
            ImGui::EndCombo();
        }

        changed |= ImGui::InputInt("SubPass Index", &pipelineDesc.SubPassIndex);
        changed |= ImGuiEnumCombo("Topology", pipelineDesc.Topology);

        if (changed)
        {
            m_pipeline = AssetManager::SetAsset(Handle<IPipeline>::Create(pipelineDesc), m_key);
            AssetManager::SaveAsset(m_pipeline, m_key);
        }
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
            if (fpath.extension().string() == ".pipeline")
            {
                m_key        = path;
                m_pipeline = AssetManager::GetAsset<IPipeline>(m_key);
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::End();
}

void PipelineEditorLayer::OnEvent(Event& event) {}

} // namespace Yogi
