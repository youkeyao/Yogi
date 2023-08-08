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
        struct Statistics
        {
            int draw_calls = 0;
            int vertices_count = 0;
            int indices_count = 0;
        };
        struct SceneData
        {
            glm::mat4 projection_view_matrix = glm::mat4(1.0f);
            glm::vec3 view_pos = glm::vec3(0.0f);
            int direction_light_num = 0;
            glm::vec4 directional_light_color = glm::vec4(0.0f);
            glm::vec3 directional_light_direction = glm::vec3(0.0f);
            int spot_light_num = 0;
            struct SpotLight {
                glm::vec4 color = glm::vec4(0.0f);
                glm::vec3 pos = glm::vec3(0.0f);
                float cutoff = 0.0f;
            } spot_lights[4];
            struct PointLight {
                glm::vec3 pos = glm::vec3(0.0f);
                float attenuation_parm = 1.0f;
                glm::vec4 color = glm::vec4(0.0f);
            } point_lights[4];
            int point_light_num = 0;
        };
        
        static void init();
        static void shutdown();

        static void set_projection_view_matrix(glm::mat4 projection_view_matrix);
        static void set_view_pos(glm::vec3 view_pos);
        static void set_sky_box(const Ref<Texture>& texture);
        static void reset_lights();
        static void set_directional_light(glm::vec4 color, glm::vec3 direction);
        static void add_spot_light(SceneData::SpotLight light);
        static void add_point_light(SceneData::PointLight light);

        static void reset_stats();
        static Statistics get_stats();

        static void flush();
        static void draw_mesh(const Ref<Mesh>& mesh, const Ref<Material>& material, const glm::mat4& transform, uint32_t entity_id);
        static void draw_skybox(const glm::mat4& camera_transform);
    private:
        static void set_pipeline(const Ref<Pipeline>& pipeline);
        static void flush_pipeline(const Ref<Pipeline>& pipeline);
    };

}