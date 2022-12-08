#pragma once

#include "renderer/renderer_command.h"
#include "renderer/orthographic_camera.h"
#include "renderer/shader.h"

namespace hazel
{

    class Renderer
    {
    public:
        static void begin_scene(OrthographicCamera& camera);
        static void end_scene();

        static void submit(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vertex_array);

        inline static RendererAPI::API get_api() { return RendererAPI::get_api(); }
    private:
        struct SceneData
        {
            glm::mat4 view_projection_matrix;
        };

        static SceneData* m_scene_data;
    };

}