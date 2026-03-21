#pragma once

#include <Yogi.h>

#include "Utils/EditorCamera.h"

namespace Yogi
{

class ViewportLayer : public Layer
{
public:
    ViewportLayer();
    virtual ~ViewportLayer() = default;

    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& event) override;

    // void open_project();
    // void open_scene(const std::string& path);
    // void save_scene();

    inline Ref<World> GetWorld() const { return Ref<World>::Create(m_world); }
    inline Entity     GetSelectedEntity() const { return m_selectedEntity; }

    void SetSelectedEntity(Entity entity) { m_selectedEntity = entity; }

private:
    void OnGUI();
    void OnToolbar();
    // void scene_info_update();
    // void toolbar_update();
    // void on_window_resized(WindowResizeEvent& e);
    // bool on_mouse_button_pressed(MouseButtonPressedEvent& e);
    // bool on_key_pressed(KeyPressedEvent& e);

private:
    Owner<World> m_world;
    Entity        m_selectedEntity;
    // Ref<SceneHierarchyPanel> m_hierarchy_panel;
    // Ref<ContentBrowserPanel> m_content_browser_panel;
    // Ref<MaterialEditorPanel> m_material_editor_panel;

    // Ref<RenderSystem> m_editor_render_system;
    // EditorCamera      m_editor_camera;

    EditorCamera                m_editorCamera;
    Owner<ForwardRenderSystem> m_editRenderSystem;

    bool    m_viewportHovered = false;
    Vector2 m_viewportSize    = { 1, 1 };
    Vector2 m_viewportBounds[2];

    Ref<IRenderPass>            m_entityIDRenderPass  = nullptr;
    Ref<ITexture>               m_frameTexture        = nullptr;
    Ref<IShaderResourceBinding> m_frameTextureBinding = nullptr;
    // ImGuizmo::OPERATION m_gizmo_type = ImGuizmo::OPERATION::TRANSLATE;

    // Ref<RenderTexture> m_frame_texture;
    // Ref<RenderTexture> m_entity_id_texture;
    // Ref<FrameBuffer>   m_frame_buffer;
    // Ref<FrameBuffer>   m_entity_frame_buffer;
    // Ref<Material>      m_entity_id_mat;

    enum class SceneState
    {
        Edit = 0,
        Play = 1
    };
    SceneState m_sceneState = SceneState::Edit;
};

} // namespace Yogi
