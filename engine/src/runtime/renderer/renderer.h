#pragma once

#include <glm/glm.hpp>
#include "runtime/resources/mesh_manager.h"
#include "runtime/resources/texture_manager.h"
#include "runtime/resources/material_manager.h"
#include "runtime/resources/pipeline_manager.h"

namespace Yogi {

    class Renderer
    {
    public:
        static void init();
        static void shutdown();
        static void on_window_resize(uint32_t width, uint32_t height);

        static void set_projection_view_matrix(glm::mat4 projection_view_matrix);

        static void flush();
        static void draw_mesh(const Ref<Mesh>& mesh, const Ref<Material>& material, const glm::mat4& transform);
    private:
        static void set_pipeline(const Ref<Pipeline>& pipeline);
        static void flush_pipeline(const Ref<Pipeline>& pipeline);
    };

}