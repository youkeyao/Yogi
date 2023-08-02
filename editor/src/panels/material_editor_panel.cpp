#include "panels/material_editor_panel.h"
#include <imgui.h>

namespace Yogi {

    MaterialEditorPanel::MaterialEditorPanel()
    {}

    void MaterialEditorPanel::on_imgui_render()
    {
        ImGui::Begin("Material Editor");
        float cursor_y = ImGui::GetCursorPosY();

        if (m_material) {
            ImGui::LabelText("", "%s", m_material->get_name().c_str());
            if (ImGui::BeginCombo("pipeline", m_material->get_pipeline()->get_name().c_str())) {
                PipelineManager::each_pipeline([&](const Ref<Pipeline>& each_pipeline){
                    bool is_selected = m_material->get_pipeline() == each_pipeline;
                    if (ImGui::Selectable(each_pipeline->get_name().c_str(), is_selected)) {
                        m_material->set_pipeline(each_pipeline);
                        MaterialManager::save_material(m_parent_path, m_material);
                    }
                });
                ImGui::EndCombo();
            }
            ImGui::Separator();
            PipelineLayout vertex_layout = m_material->get_pipeline()->get_vertex_layout();
            uint8_t* data = m_material->get_data();
            std::vector<std::pair<uint32_t, Ref<Texture>>> textures = m_material->get_textures();
            uint32_t texture_index = 0;
            for (auto& element : vertex_layout.get_elements()) {
                if (element.name == "a_Position" || element.name == "a_TexCoord" || element.name == "a_EntityID") {
                    continue;
                }
                else if (element.name.substr(0, 4) == "TEX_") {
                    Ref<Texture> texture = textures[texture_index].second;
                    if (ImGui::BeginCombo(element.name.c_str(), texture ? texture->get_name().c_str() : "")) {
                        TextureManager::each_texture([&](const Ref<Texture2D>& each_texture){
                            bool is_selected = texture == each_texture;
                            if (ImGui::Selectable(each_texture->get_name().c_str(), is_selected)) {
                                m_material->set_texture(texture_index, each_texture);
                                MaterialManager::save_material(m_parent_path, m_material);
                            }
                        });
                        TextureManager::each_render_texture([&](const Ref<RenderTexture>& each_texture){
                            bool is_selected = texture == each_texture;
                            if (ImGui::Selectable(each_texture->get_name().c_str(), is_selected)) {
                                m_material->set_texture(texture_index, each_texture);
                                MaterialManager::save_material(m_parent_path, m_material);
                            }
                        });
                        if (ImGui::Selectable("none", texture == nullptr)) {
                            m_material->set_texture(texture_index, nullptr);
                            MaterialManager::save_material(m_parent_path, m_material);
                        }
                        ImGui::EndCombo();
                    }
                }
                else if (element.type == ShaderDataType::Float4) {
                    if (ImGui::ColorEdit4(element.name.c_str(), (float*)(data + element.offset))) {
                        MaterialManager::save_material(m_parent_path, m_material);
                    }
                }
            }
        }

        // blank space
        ImGui::SetCursorPosY(cursor_y);
        ImVec2 viewport_region_min = ImGui::GetWindowContentRegionMin();
        ImVec2 viewport_region_max = ImGui::GetWindowContentRegionMax();
        ImGui::InvisibleButton("blank", { std::max(viewport_region_max.x - viewport_region_min.x, 1.0f), std::max(viewport_region_max.y - viewport_region_min.y, 1.0f) });
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("content_browser_item")) {
                const char* path = (const char*)payload->Data;
                m_material = MaterialManager::get_material(std::filesystem::path{path}.stem().string());
                m_parent_path = std::filesystem::path{path}.parent_path().string();
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::End();
    }

}