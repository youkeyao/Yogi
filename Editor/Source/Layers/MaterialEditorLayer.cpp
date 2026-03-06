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
        bool changed = false;

        ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_FramePadding;
        int                removeIndex   = -1;
        for (int i = 0; i < m_material->GetPasses().size(); ++i)
        {
            auto pass     = m_material->GetPasses()[i];
            bool isOpened = ImGui::TreeNodeEx(("Pass" + std::to_string(i)).c_str(), treeNodeFlags);
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Delete Pass"))
                {
                    removeIndex = i;
                }
                ImGui::EndPopup();
            }
            if (isOpened)
            {
                std::string renderPassKey = AssetManager::GetAssetKey(pass.RenderPass);
                if (ImGui::BeginCombo("Render Pass", renderPassKey.c_str()))
                {
                    for (auto& key : AssetRegistry::GetKeys<IRenderPass>())
                    {
                        if (ImGui::Selectable(key.c_str(), key == renderPassKey))
                        {
                            pass.RenderPass = AssetManager::GetAsset<IRenderPass>(key);
                            changed |= true;
                        }
                    }
                    ImGui::EndCombo();
                }
                if (ImGui::TreeNodeEx("Shaders", treeNodeFlags))
                {
                    std::vector<std::string>::iterator removeIt = pass.ShaderKeys.end();
                    for (auto it = pass.ShaderKeys.begin(); it != pass.ShaderKeys.end(); ++it)
                    {
                        auto& shaderKey = *it;
                        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                        if (ImGui::BeginCombo(("##" + shaderKey).c_str(), shaderKey.c_str()))
                        {
                            for (auto& key : AssetRegistry::GetKeys<ShaderDesc>())
                            {
                                if (ImGui::Selectable(key.c_str(), key == shaderKey))
                                {
                                    shaderKey = key;
                                    changed |= true;
                                }
                            }
                            ImGui::EndCombo();
                        }
                        ImGui::PopItemWidth();
                        if (ImGui::BeginPopupContextItem())
                        {
                            if (ImGui::MenuItem("Delete Shader"))
                            {
                                removeIt = it;
                                changed |= true;
                            }
                            ImGui::EndPopup();
                        }
                    }
                    if (removeIt != pass.ShaderKeys.end())
                    {
                        pass.ShaderKeys.erase(removeIt);
                    }
                    if (ImGui::Button("+", { ImGui::GetContentRegionAvail().x, 0.0f }))
                    {
                        pass.ShaderKeys.push_back("");
                        changed |= true;
                    }
                    ImGui::TreePop();
                }
                changed |= ImGuiMaterialData(pass.ShaderKeys, pass.PassData);

                m_material->SetPass(i, pass);
                ImGui::TreePop();
            }
        }
        if (removeIndex >= 0)
        {
            m_material->RemovePass(removeIndex);
            changed |= true;
        }

        if (ImGui::Button("+", { ImGui::GetContentRegionAvail().x, 0.0f }))
        {
            auto shaderResourceBinding = Yogi::ResourceManager::GetResource<Yogi::IShaderResourceBinding>(
                std::vector<Yogi::ShaderResourceAttribute>{ Yogi::ShaderResourceAttribute{
                    0, 1, Yogi::ShaderResourceType::Buffer, Yogi::ShaderStage::Vertex } });
            m_material->AddPass({ nullptr, {}, {} });
            changed |= true;
        }

        if (changed)
        {
            AssetManager::SaveAsset(m_material, m_key);
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
            if (fpath.extension().string() == ".mat")
            {
                m_key      = path;
                m_material = AssetManager::GetAsset<Material>(m_key);
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
