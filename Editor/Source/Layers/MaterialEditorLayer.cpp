#include "Layers/MaterialEditorLayer.h"

#include "Registry/AssetRegistry.h"

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
        ImGui::LabelText("", "%s", m_key.c_str());
        auto pipelineDesc = m_material->GetPipeline()->GetDesc();
        for (auto& shader : pipelineDesc.Shaders)
        {
            std::string shaderKey = AssetManager::GetAssetKey(shader);
            if (ImGui::BeginCombo(shaderKey.c_str(), shaderKey.c_str()))
            {
                for (auto& key : AssetRegistry::GetKeys<ShaderDesc>())
                {
                    if (ImGui::Selectable(key.c_str(), key == shaderKey))
                    {
                        shader = AssetManager::GetAsset<ShaderDesc>(key);
                    }
                }
                ImGui::EndCombo();
            }
        }
        if (ImGui::Button("+", { ImGui::GetContentRegionAvail().x, 0.0f }))
            ImGui::OpenPopup("AddShader");
        if (ImGui::BeginPopup("AddShader"))
        {
            for (auto& key : AssetRegistry::GetKeys<ShaderDesc>())
            {
                if (ImGui::MenuItem(key.c_str()))
                {
                    pipelineDesc.Shaders.push_back(AssetManager::GetAsset<ShaderDesc>(key));
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }
        ImGui::Separator();
        auto&    vertexLayout = m_material->GetPipeline()->GetDesc().VertexLayout;
        auto&    data         = m_material->GetData();
        auto&    textures     = m_material->GetTextures();
        uint32_t textureIndex = 0;
        for (auto& element : vertexLayout)
        {
            if (element.Name == "a_Position" || element.Name == "a_Normal" || element.Name == "a_TexCoord" ||
                element.Name == "a_EntityID")
            {
                continue;
            }
            else if (element.Name.substr(0, 4) == "TEX_")
            {
                Ref<ITexture> texture    = textures[textureIndex].second;
                std::string   textureKey = texture ? AssetManager::GetAssetKey(texture) : "None";
                if (ImGui::BeginCombo(element.Name.c_str(), textureKey.c_str()))
                {
                    // TextureManager::each_texture([&](const Ref<Texture2D>& each_texture) {
                    //     bool is_selected = texture == each_texture;
                    //     if (ImGui::Selectable(each_texture->get_name().c_str(), is_selected))
                    //     {
                    //         m_material->set_texture(textureIndex, each_texture);
                    //         MaterialManager::save_material(m_parent_path, m_material);
                    //     }
                    // });
                    // TextureManager::each_render_texture([&](const Ref<RenderTexture>& each_texture) {
                    //     bool is_selected = texture == each_texture;
                    //     if (ImGui::Selectable(each_texture->get_name().c_str(), is_selected))
                    //     {
                    //         m_material->set_texture(textureIndex, each_texture);
                    //         MaterialManager::save_material(m_parent_path, m_material);
                    //     }
                    // });
                    if (ImGui::Selectable("None", texture))
                    {
                        m_material->SetTexture(textureIndex, nullptr);
                        AssetManager::SaveAsset(m_material, m_key);
                    }
                    ImGui::EndCombo();
                }
                textureIndex++;
            }
            else if (element.Type == ShaderElementType::Float)
            {
                if (ImGui::InputFloat(element.Name.c_str(), (float*)(data.data() + element.Offset)))
                {
                    AssetManager::SaveAsset(m_material, m_key);
                }
            }
            else if (element.Type == ShaderElementType::Float2)
            {
                if (ImGui::InputFloat2(element.Name.c_str(), (float*)(data.data() + element.Offset)))
                {
                    AssetManager::SaveAsset(m_material, m_key);
                }
            }
            else if (element.Type == ShaderElementType::Float3)
            {
                if (ImGui::InputFloat3(element.Name.c_str(), (float*)(data.data() + element.Offset)))
                {
                    AssetManager::SaveAsset(m_material, m_key);
                }
            }
            else if (element.Type == ShaderElementType::Float4)
            {
                if (ImGui::ColorEdit4(element.Name.c_str(), (float*)(data.data() + element.Offset)))
                {
                    AssetManager::SaveAsset(m_material, m_key);
                }
            }
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
                m_key      = fpath.generic_string();
                m_material = AssetManager::GetAsset<Material>(m_key);
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::End();
}

void MaterialEditorLayer::OnEvent(Event& event) {}

} // namespace Yogi
