#include "editor_layer.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace Yogi {

    EditorLayer::EditorLayer() : Layer("Sandbox 2D"), m_camera_controller(1280.0f / 720.0f) {}

    void EditorLayer::on_attach()
    {
        YG_PROFILE_FUNCTION();

        m_checkerboard_texture = Texture2D::create("../sandbox/assets/textures/checkerboard.png");

        FrameBufferProps props;
        props.width = 1280;
        props.height = 720;
        m_frame_buffer = FrameBuffer::create(props);
    }

    void EditorLayer::on_detach()
    {
        YG_PROFILE_FUNCTION();
    }

    void EditorLayer::on_update(Timestep ts)
    {
        YG_PROFILE_FUNCTION();

        m_camera_controller.on_update(ts);

        Renderer2D::reset_stats();
        {
            YG_PROFILE_SCOPE("Render prep");
            m_frame_buffer->bind();
            RenderCommand::set_clear_color({ 0.1f, 0.1f, 0.1f, 1.0f });
            RenderCommand::clear();
        }

        {
            YG_PROFILE_SCOPE("Render draw");
            Renderer2D::begin_scene(m_camera_controller.get_camera());
            Renderer2D::draw_quad({-1.0f, 0.0f}, glm::radians(45.0f), {0.8f, 0.8f}, {0.8f, 0.2f, 0.3f, 1.0f});
            Renderer2D::draw_quad({0.5f, -0.5f}, {0.5f, 0.75f}, {0.2f, 0.3f, 0.8f, 1.0f});
            Renderer2D::draw_quad({-0.5f, 0.5f}, {0.5f, 0.75f}, {0.2f, 0.8f, 0.3f, 1.0f});
            Renderer2D::draw_quad({ 0.0f, 0.0f, -0.1f }, { 20.0f, 20.0f }, m_checkerboard_texture, {{0.0f, 0.0f}, {10.0f, 10.0f}}, m_square_color);
            Renderer2D::draw_quad({ 0.0f, 0.0f, 0.1f }, glm::radians(45.0f), { 1.0f, 1.0f }, m_checkerboard_texture, {{0.0f, 0.0f}, {5.0f, 5.0f}});

            for (float y = -5.0f; y < 5.0f; y += 0.5f) {
                for (float x = -5.0f; x < 5.0f; x += 0.5f) {
                    Renderer2D::draw_quad({x, y}, {0.45f, 0.45f}, {(x + 5.0f) / 10.0f, 0.4f, (y + 5.0f) / 10.0f, 0.7f});
                }
            }
            Renderer2D::end_scene();
        }

        m_frame_buffer->unbind();
    }

    void EditorLayer::on_imgui_render()
    {
        YG_PROFILE_FUNCTION();
        
        Renderer2D::Statistics stats = Renderer2D::get_stats();

        static bool p_open = true;
        static bool opt_fullscreen = true;
        static bool opt_padding = false;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen)
        {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }
        else
        {
            dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
        }

        // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
        // and handle the pass-thru hole, so we ask Begin() to not render a background.
        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
        // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
        // all active windows docked into it will lose their parent and become undocked.
        // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
        // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
        if (!opt_padding)
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Demo", &p_open, window_flags);
        if (!opt_padding)
            ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

        // Submit the DockSpace
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Exit"))
                    Application::get().close();
                
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        ImGui::Begin("Settings");
        ImGui::Text("Renderer2D Stats:");
        ImGui::Text("Draw Calls: %d", stats.draw_calls);
        ImGui::Text("Quads: %d", stats.quad_count);
        ImGui::Text("Vertices: %d", stats.get_total_vertex_count());
        ImGui::Text("Indices: %d", stats.get_total_index_count());
        ImGui::ColorEdit4("Square color", glm::value_ptr(m_square_color));

        uint64_t texture_id = m_frame_buffer->get_color_attachment();
        ImGui::Image((void*)texture_id, ImVec2(320.0f, 180.0f), ImVec2( 0, 1 ), ImVec2( 1, 0 ));

        ImGui::End();

        ImGui::End();
    }

    void EditorLayer::on_event(Event& e)
    {
        m_camera_controller.on_event(e);
    }

}