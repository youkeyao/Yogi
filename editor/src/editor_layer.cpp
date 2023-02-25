#include "editor_layer.h"
#include "imgui_config.h"
#include "editor_camera_controller_system.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace Yogi {

    static uint32_t s_max_viewport_size = 4096;

    EditorLayer::EditorLayer() : Layer("Sandbox 2D") {}

    void EditorLayer::on_attach()
    {
        YG_PROFILE_FUNCTION();

        // m_checkerboard_texture = Texture2D::create("../sandbox/assets/textures/checkerboard.png");

        // m_frame_texture = Texture2D::create(s_max_viewport_size, s_max_viewport_size);
        // m_frame_buffer = FrameBuffer::create(s_max_viewport_size, s_max_viewport_size, { m_frame_texture });

        // m_scene = CreateRef<Scene>();
        // m_hierarchy_panel = CreateRef<SceneHierarchyPanel>(m_scene);

        // Ref<Entity> square = m_scene->create_entity();
        // square->add_component<TagComponent>("square");
        // square->add_component<TransformComponent>();
        // SpriteRendererComponent sprite;
        // sprite.texture = m_checkerboard_texture;
        // sprite.color = { 0.8f, 0.2f, 0.3f, 1.0f };
        // square->add_component<SpriteRendererComponent>(sprite);

        // m_editor_camera = m_scene->create_entity();
        // m_editor_camera->add_component<TagComponent>("camera");
        // m_editor_camera->add_component<TransformComponent>();
        // m_editor_camera->add_component<CameraComponent>();

        // m_scene->register_system<CameraSystem>();
        // m_scene->register_system<RenderSystem>();

        // imgui_init();
    }

    void EditorLayer::on_detach()
    {
        YG_PROFILE_FUNCTION();

        ImGui_Renderer_Shutdown();
        ImGui_Window_Shutdown();

        ImGui::DestroyContext();
    }

    void EditorLayer::on_update(Timestep ts)
    {
        YG_PROFILE_FUNCTION();
        ComponentBase<int, float> test("TsetComponent", {"int", "float"}, 1, 2.0);
        test.field<float>("float") = 3.0;
        auto s = sizeof(test);
        auto t = test.field<float>("float");
        test.each_field([](std::string name, const std::type_info& type){
            YG_CORE_INFO("{0} {1}", name, type.name());
        });
        // auto t = get_field(test, "int");
        
        // imgui_begin();
        // m_hierarchy_panel->on_imgui_render();

        // Renderer2D::reset_stats();
        // m_frame_buffer->bind();

        // if (m_viewport_focused) {
        //     EditorCameraControllerSystem::on_update(ts, m_editor_camera);
        // }
        // m_scene->on_update(ts);

        // m_frame_buffer->unbind();

        // imgui_end();
    }

    void EditorLayer::imgui_init()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        (void) io;

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui::StyleColorsDark();

        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        Application& app = Application::get();
        ImGui_Window_Init(app.get_window().get_native_window(), true);
        ImGui_Renderer_Init("#version 330");
    }

    void EditorLayer::imgui_begin()
    {
        YG_PROFILE_FUNCTION();

        // ImGui_Renderer_NewFrame();
        // ImGui_Window_NewFrame();
        // ImGui::NewFrame();
        
        // Renderer2D::Statistics stats = Renderer2D::get_stats();

        // static bool p_open = true;
        // static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

        // ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        // const ImGuiViewport* viewport = ImGui::GetMainViewport();
        // ImGui::SetNextWindowPos(viewport->WorkPos);
        // ImGui::SetNextWindowSize(viewport->WorkSize);
        // ImGui::SetNextWindowViewport(viewport->ID);
        // ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        // ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        // window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        // window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        // if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        //     window_flags |= ImGuiWindowFlags_NoBackground;

        // ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        // ImGui::Begin("DockSpace Demo", &p_open, window_flags);
        // ImGui::PopStyleVar();

        // ImGui::PopStyleVar(2);

        // // Submit the DockSpace
        // ImGuiIO& io = ImGui::GetIO();
        // if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        //     ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        //     ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        // }

        // if (ImGui::BeginMenuBar()) {
        //     if (ImGui::BeginMenu("File")) {
        //         if (ImGui::MenuItem("Exit"))
        //             Application::get().close();
                
        //         ImGui::EndMenu();
        //     }

        //     ImGui::EndMenuBar();
        // }

        // ImGui::Begin("Settings");
        // ImGui::Text("Renderer2D Stats:");
        // ImGui::Text("Draw Calls: %d", stats.draw_calls);
        // ImGui::Text("Quads: %d", stats.quad_count);
        // ImGui::Text("Vertices: %d", stats.get_total_vertex_count());
        // ImGui::Text("Indices: %d", stats.get_total_index_count());
        // ImGui::ColorEdit4("Square color", glm::value_ptr(m_square_color));
        // ImGui::End();

        // ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        // ImGui::Begin("Viewport");
        // m_viewport_focused = ImGui::IsWindowFocused();
        // ImVec2 viewport_region_min = ImGui::GetWindowContentRegionMin();
        // ImVec2 viewport_region_max = ImGui::GetWindowContentRegionMax();
        // ImVec2 viewport_offset = ImGui::GetWindowPos();
        // m_viewport_bounds[0] = { viewport_region_min.x + viewport_offset.x, viewport_region_min.y + viewport_offset.y };
        // m_viewport_bounds[1] = { viewport_region_max.x + viewport_offset.x, viewport_region_max.y + viewport_offset.y };
        // glm::vec2 new_viewport_size = { viewport_region_max.x - viewport_region_min.x, viewport_region_max.y - viewport_region_min.y };
        // if ((uint32_t)new_viewport_size.x < 0 || (uint32_t)new_viewport_size.y < 0 ||
        //     (uint32_t)new_viewport_size.x > s_max_viewport_size || (uint32_t)new_viewport_size.y > s_max_viewport_size
        // ) {
        //     YG_CORE_WARN("Invalid viewport size!");
        // }
        // else if (new_viewport_size.x != m_viewport_size.x || new_viewport_size.y != m_viewport_size.y) {
        //     m_viewport_size = new_viewport_size;
        //     WindowResizeEvent e((uint32_t)m_viewport_size.x, (uint32_t)m_viewport_size.y);
        //     m_scene->on_event(e);
        //     EditorCameraControllerSystem::on_event(e, m_editor_camera);
        //     RenderCommand::set_viewport(0.0f, 0.0f, m_viewport_size.x, m_viewport_size.y);
        // }
        // ImGui::Image(
        //     (void*)(uint64_t)m_frame_texture->get_renderer_id(),
        //     ImVec2(m_viewport_size.x, m_viewport_size.y),
        //     ImVec2( 0, m_viewport_size.y / m_frame_texture->get_height() ),
        //     ImVec2( m_viewport_size.x / m_frame_texture->get_width(), 0 )
        // );
        // ImGui::End();
        // ImGui::PopStyleVar();

        // ImGui::End();
    }

    void EditorLayer::imgui_end()
    {
        YG_PROFILE_FUNCTION();

        ImGuiIO& io = ImGui::GetIO();
        Application& app = Application::get();
        io.DisplaySize = ImVec2(app.get_window().get_width(), app.get_window().get_height());

        ImGui::Render();
        ImGui_Renderer_Draw(ImGui::GetDrawData());
        ImGui_Window_Render();
    }

    void EditorLayer::on_event(Event& e)
    {
        // if (e.get_event_type() != WindowResizeEvent::get_static_type()) {
        //     if (m_viewport_focused)
        //         EditorCameraControllerSystem::on_event(e, m_editor_camera);
        //     m_scene->on_event(e);
        // }
    }

}