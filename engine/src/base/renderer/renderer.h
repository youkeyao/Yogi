#pragma once

#include "base/renderer/renderer_command.h"
#include "base/renderer/orthographic_camera.h"
#include "base/renderer/shader.h"

namespace hazel
{

    class Renderer
    {
    public:
        static void init();
        static void shutdown();

        static void on_window_resize(uint32_t width, uint32_t height);
        
        static void begin_scene(OrthographicCamera& camera);
        static void end_scene();

        static void submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertex_array, const glm::mat4& transform = glm::mat4(1.0f));
    private:
        struct SceneData
        {
            glm::mat4 view_projection_matrix;
        };

        static SceneData* m_scene_data;
    };

}