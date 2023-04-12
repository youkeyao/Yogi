#include "runtime/renderer/renderer.h"
#include "runtime/renderer/render_command.h"
#include "runtime/renderer/buffer.h"
#include "runtime/core/application.h"

namespace Yogi {

    struct Vertex
    {
        glm::vec3 position;
        glm::vec4 color;
        glm::vec2 texcoord;
        int texid;
    };

    struct RendererData
    {
        static const uint32_t max_triangles = 10000;
        static const uint32_t max_vertices = max_triangles * 3;
        static const uint32_t max_vertices_size = max_vertices * 40;
        static const uint32_t max_indices = max_triangles * 3;
        static const uint32_t max_texture_slots = 32;

        Ref<VertexBuffer> mesh_vertex_buffer;
        Ref<IndexBuffer> mesh_index_buffer;
        Ref<UniformBuffer> scene_uniform_buffer;
        Ref<Pipeline> render_pipeline;

        Ref<Pipeline> now_pipeline;
        uint8_t** now_vertices_base;
        uint8_t** now_vertices_cur;
        uint32_t** now_indices_base;
        uint32_t** now_indices_cur;
        uint32_t* now_texture_slot_index;
        std::array<Ref<Texture2D>, max_texture_slots>* now_texture_slots;

        std::map<Ref<Pipeline>, uint8_t*> mesh_vertices_bases;
        std::map<Ref<Pipeline>, uint8_t*> mesh_vertices_curs;

        std::map<Ref<Pipeline>, uint32_t*> mesh_indices_bases;
        std::map<Ref<Pipeline>, uint32_t*> mesh_indices_curs;

        std::map<Ref<Pipeline>, uint32_t> mesh_texture_slot_indexs;
        std::map<Ref<Pipeline>, std::array<Ref<Texture2D>, max_texture_slots>> mesh_texture_slots;

        struct SceneData
        {
            glm::mat4 projection_view_matrix = glm::mat4(1.0f);
        };
        SceneData scene_data;
    };

    static RendererData* s_data;

    void Renderer::init()
    {
        YG_PROFILE_FUNCTION();

        s_data = new RendererData();

        s_data->mesh_vertex_buffer = VertexBuffer::create(nullptr, RendererData::max_vertices * sizeof(Vertex), false);
        s_data->mesh_vertex_buffer->bind();
        s_data->mesh_index_buffer = IndexBuffer::create(nullptr, RendererData::max_indices, false);
        s_data->mesh_index_buffer->bind();
        s_data->scene_uniform_buffer = UniformBuffer::create(sizeof(RendererData::SceneData));
    }

    void Renderer::shutdown()
    {
        YG_PROFILE_FUNCTION();

        for (auto& [pipeline_name, vertices_base] : s_data->mesh_vertices_bases) {
            delete[] vertices_base;
        }
        for (auto& [pipeline_name, indices_base] : s_data->mesh_indices_bases) {
            delete[] indices_base;
        }

        delete s_data;
    }

    void Renderer::on_window_resize(uint32_t width, uint32_t height)
    {
        RenderCommand::set_viewport(0, 0, width, height);
    }

    void Renderer::set_pipeline(const Ref<Pipeline>& pipeline)
    {
        if (pipeline != s_data->render_pipeline) {
            s_data->render_pipeline = pipeline;
            s_data->render_pipeline->bind();
            s_data->scene_uniform_buffer->bind(0);
            for (int32_t i = 0; i < *s_data->now_texture_slot_index; i ++) {
                auto& texture = (*s_data->now_texture_slots)[i];
                texture->bind(1, i);
            }
        }
    }

    void Renderer::set_projection_view_matrix(glm::mat4 projection_view_matrix)
    {
        s_data->scene_data.projection_view_matrix = projection_view_matrix;
        s_data->scene_uniform_buffer->set_data(&s_data->scene_data, sizeof(RendererData::SceneData));
    }

    void Renderer::flush_pipeline(const Ref<Pipeline>& pipeline)
    {
        YG_PROFILE_FUNCTION();

        s_data->now_indices_base = &(s_data->mesh_indices_bases[pipeline]);
        s_data->now_indices_cur = &(s_data->mesh_indices_curs[pipeline]);
        if (pipeline && *s_data->now_indices_base != *s_data->now_indices_cur) {
            set_pipeline(pipeline);
            s_data->now_vertices_base = &(s_data->mesh_vertices_bases[pipeline]);
            s_data->now_vertices_cur = &(s_data->mesh_vertices_curs[pipeline]);
            s_data->now_texture_slot_index = &(s_data->mesh_texture_slot_indexs[pipeline]);
            s_data->now_texture_slots = &(s_data->mesh_texture_slots[pipeline]);

            uint32_t vertices_size = (uint32_t)(*(s_data->now_vertices_cur) - *(s_data->now_vertices_base));
            s_data->mesh_vertex_buffer->set_data(*(s_data->now_vertices_base), vertices_size);
            uint32_t indices_size = (uint32_t)((uint8_t*)*(s_data->now_indices_cur) - (uint8_t*)*(s_data->now_indices_base));
            s_data->mesh_index_buffer->set_data(*(s_data->now_indices_base), indices_size);
            RenderCommand::draw_indexed(indices_size / sizeof(uint32_t));

            *(s_data->now_vertices_cur) = *(s_data->now_vertices_base);
            *(s_data->now_indices_cur) = *(s_data->now_indices_base);
        }
    }

    void Renderer::flush()
    {
        YG_PROFILE_FUNCTION();

        for (auto& [pipeline, texture_slot_index] : s_data->mesh_texture_slot_indexs) {
            flush_pipeline(pipeline);
        }
    }

    void Renderer::draw_mesh(const Ref<Mesh>& mesh, const Ref<Material>& material, const glm::mat4& transform)
    {
        std::vector<std::pair<uint32_t, Ref<Texture2D>>> textures = material->get_textures();
        Ref<Pipeline> pipeline = material->get_pipeline();
        if (s_data->now_pipeline != pipeline) {
            if (s_data->mesh_vertices_bases.find(pipeline) == s_data->mesh_vertices_bases.end()) {
                s_data->mesh_vertices_curs[pipeline] = s_data->mesh_vertices_bases[pipeline] = new uint8_t[RendererData::max_vertices_size];
                s_data->mesh_indices_curs[pipeline] = s_data->mesh_indices_bases[pipeline] = new uint32_t[RendererData::max_indices];
                s_data->mesh_texture_slot_indexs[pipeline] = 0;
            }
            s_data->now_pipeline = pipeline;
            s_data->now_vertices_base = &(s_data->mesh_vertices_bases[pipeline]);
            s_data->now_vertices_cur = &(s_data->mesh_vertices_curs[pipeline]);
            s_data->now_indices_base = &(s_data->mesh_indices_bases[pipeline]);
            s_data->now_indices_cur = &(s_data->mesh_indices_curs[pipeline]);
            s_data->now_texture_slot_index = &(s_data->mesh_texture_slot_indexs[pipeline]);
            s_data->now_texture_slots = &(s_data->mesh_texture_slots[pipeline]);
        }

        auto& vertices_cur = *s_data->now_vertices_cur;
        auto& indices_cur = *s_data->now_indices_cur;
        auto& vertices_base = *s_data->now_vertices_base;
        auto& indices_base = *s_data->now_indices_base;
        auto& texture_slot_index = *s_data->now_texture_slot_index;
        auto& texture_slots = *s_data->now_texture_slots;

        PipelineLayout vertex_layout = pipeline->get_vertex_layout();

        if (vertices_cur - vertices_base + vertex_layout.get_stride() * mesh->vertices.size() >= RendererData::max_vertices_size ||
            indices_cur - indices_base + mesh->indices.size() >= RendererData::max_indices ||
            texture_slot_index + textures.size() >= RendererData::max_texture_slots
        ) {
            flush_pipeline(pipeline);
            texture_slot_index = 0;
        }

        uint32_t vertices_count = (vertices_cur - vertices_base) / vertex_layout.get_stride();

        for (auto& [pos, texcoord] : mesh->vertices) {
            memcpy(vertices_cur, material->get_data(), vertex_layout.get_stride());
            int32_t position_offset = material->get_position_offset();
            int32_t texcoord_offset = material->get_texcoord_offset();
            if (position_offset >= 0) {
                glm::vec4 position{pos.x, pos.y, pos.z, 1.0f};
                position = transform * position;
                memcpy(vertices_cur + position_offset, &position, 12);
            }
            if (texcoord_offset >= 0) {
                memcpy(vertices_cur + texcoord_offset, &texcoord, 8);
            }
            for (auto& [texture_offset, texture] : textures) {
                int32_t texture_index = -1;
                if (texture) {
                    for (uint32_t i = 0; i < texture_slot_index; i ++) {
                        if (texture_slots[i] == texture) {
                            texture_index = i;
                            break;
                        }
                    }
                    if (texture_index == -1) {
                        texture_index = texture_slot_index;
                        texture_slots[texture_index] = texture;
                        texture_slot_index ++;
                        s_data->render_pipeline = nullptr;
                    }
                }
                memcpy(vertices_cur + texture_offset, &texture_index, 4);
            }
            vertices_cur += vertex_layout.get_stride();
        }
        for (auto& index : mesh->indices) {
            *indices_cur = vertices_count + index;
            indices_cur ++;
        }
    }

}