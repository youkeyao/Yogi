#include "panels/content_browser_panel.h"
#include "panels/fontawesome4_header.h"
#include <imgui.h>
#include <imgui_internal.h>

namespace Yogi {

    ContentBrowserPanel::ContentBrowserPanel(const std::string& dir)
    {
        m_current_directory = m_base_directory = dir;
    }

    void ContentBrowserPanel::on_imgui_render()
    {
        ImGui::Begin("Content Browser");
        float panel_width = ImGui::GetContentRegionAvail().x;
        static float padding = 8.0f;

        // new content
        if (ImGui::BeginPopupContextWindow()) {
            if (ImGui::MenuItem("Create Material")) {
                Ref<Material> material = Material::create("new_material", PipelineManager::get_pipeline("Flat"));
                while (std::filesystem::exists(m_current_directory.string() + "/" + material->get_name() + ".mat")) {
                    material->set_name("_" + material->get_name());
                }
                MaterialManager::save_material(m_current_directory.string(), material);
            }
            ImGui::EndPopup();
        }

        // path
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::Button("Assets");

        float now_x = ImGui::GetItemRectSize().x + padding;
        if (ImGui::IsItemClicked()) {
            m_current_directory = m_base_directory;
        }
        std::list<std::filesystem::path> paths;
        for (auto p = m_current_directory; p != m_base_directory; p = p.parent_path()) {
            paths.emplace_front(p);
        }
        
        for (auto& p : paths) {
            float text_size = ImGui::CalcTextSize("/").x;
            if (now_x + text_size < panel_width) ImGui::SameLine();
            else now_x = padding;
            ImGui::Text("/");
            now_x += ImGui::GetItemRectSize().x + padding;
            text_size = ImGui::CalcTextSize(p.filename().string().c_str()).x;
            if (now_x + text_size < panel_width) ImGui::SameLine();
            else now_x = padding;
            ImGui::Button(p.filename().string().c_str());
            now_x += ImGui::GetItemRectSize().x + padding;
            if (ImGui::IsItemClicked()) {
                m_current_directory = p;
            }
        }
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f);

        // content
        static float thumbnail_size = 80.0f;
        float cell_size = thumbnail_size + padding;
        int column_count = (int)(panel_width / cell_size);
        if (column_count < 1) column_count = 1;

        ImGui::Columns(column_count, 0, false);

        for (auto& directory_entry : std::filesystem::directory_iterator(m_current_directory)) {
            const auto& path = directory_entry.path();
            std::string filename = path.filename().string();

            const char* icon = directory_entry.is_directory() ? ICON_FA_FOLDER : ICON_FA_FILE;

            ImGui::Selectable(("##" + path.string()).c_str(), false, 0, { thumbnail_size, thumbnail_size });
            auto old_pos = ImGui::GetCursorPos();
            auto icon_size = ImGui::CalcTextSize(icon);

            if (ImGui::BeginDragDropSource()) {
                auto item_path = path.string();
                ImGui::SetDragDropPayload("content_browser_item", item_path.data(), item_path.size() + 1);
                ImGui::EndDragDropSource();
            }
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                if (ImGui::IsItemHovered() && directory_entry.is_directory()) {
                    m_current_directory /= path.filename();
                }
            }

            ImGui::SetCursorPosX(old_pos.x + thumbnail_size / 2 - icon_size.x / 2);
            ImGui::SetCursorPosY(old_pos.y - cell_size / 2 + icon_size.y / 2);
            ImGui::Text("%s", icon);
            auto text_size = ImGui::CalcTextSize(filename.c_str());
            ImGui::SetCursorPosY(old_pos.y);
            if (text_size.x >= cell_size) {
                ImGui::SetCursorPosY(old_pos.y);
                ImGui::TextWrapped("%s", filename.c_str());
            }
            else {
                ImGui::SetCursorPosX(old_pos.x + thumbnail_size / 2 - text_size.x / 2);
                ImGui::Text("%s", filename.c_str());
            }

            ImGui::NextColumn();
        }

        ImGui::End();
    }

}