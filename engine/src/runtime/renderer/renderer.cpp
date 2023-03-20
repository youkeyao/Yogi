#include "runtime/renderer/renderer.h"
#include "runtime/renderer/mesh_manager.h"
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
        static const uint32_t max_triangles = 2;
        static const uint32_t max_vertices = max_triangles * 3;
        static const uint32_t max_indices = max_triangles * 3;
        static const uint32_t max_texture_slots = 32;

        Ref<VertexBuffer> mesh_vertex_buffer;
        Ref<IndexBuffer> mesh_index_buffer;
        Ref<UniformBuffer> scene_uniform_buffer;
        Ref<Pipeline> render_pipeline;

        Vertex* mesh_vertices_base = nullptr;
        Vertex* mesh_vertices_cur = nullptr;

        uint32_t* mesh_indices_base = nullptr;
        uint32_t* mesh_indices_cur = nullptr;

        uint32_t texture_slot_index = 0;
        std::array<Ref<Texture2D>, max_texture_slots> texture_slots;

        struct SceneData
        {
            glm::mat4 projection_view_matrix;
        };
        SceneData scene_data;
    };

    static RendererData* s_data;

    void Renderer::init()
    {
        YG_PROFILE_FUNCTION();

        MeshManager::init();
        s_data = new RendererData();

        s_data->mesh_vertex_buffer = VertexBuffer::create(nullptr, RendererData::max_vertices * sizeof(Vertex), false);
        s_data->mesh_vertex_buffer->bind();
        s_data->mesh_index_buffer = IndexBuffer::create(nullptr, RendererData::max_indices, false);
        s_data->mesh_index_buffer->bind();
        s_data->render_pipeline = Pipeline::create("editor");
        s_data->render_pipeline->bind();
        s_data->scene_uniform_buffer = UniformBuffer::create(sizeof(RendererData::SceneData));
        s_data->scene_uniform_buffer->bind(0);

        s_data->mesh_vertices_cur = s_data->mesh_vertices_base = new Vertex[RendererData::max_vertices];
        s_data->mesh_indices_cur = s_data->mesh_indices_base = new uint32_t[RendererData::max_indices];
    }

    void Renderer::shutdown()
    {
        YG_PROFILE_FUNCTION();

        delete[] s_data->mesh_vertices_base;
        delete[] s_data->mesh_indices_base;
        delete s_data;
    }

    void Renderer::on_window_resize(uint32_t width, uint32_t height)
    {
        RenderCommand::set_viewport(0, 0, width, height);
    }

    void Renderer::set_pipeline(Ref<Pipeline> pipeline)
    {
        s_data->render_pipeline = pipeline;
        s_data->render_pipeline->bind();
        s_data->scene_uniform_buffer->bind(0);
    }

    void Renderer::set_projection_view_matrix(glm::mat4 projection_view_matrix)
    {
        s_data->scene_data.projection_view_matrix = projection_view_matrix;
        s_data->scene_uniform_buffer->set_data(&s_data->scene_data, sizeof(RendererData::SceneData));
    }

    void Renderer::flush()
    {
        if (s_data->mesh_indices_cur != s_data->mesh_indices_base) {
            uint32_t vertices_size = (uint32_t)((uint8_t*)(s_data->mesh_vertices_cur) - (uint8_t*)s_data->mesh_vertices_base);
            s_data->mesh_vertex_buffer->set_data(s_data->mesh_vertices_base, vertices_size);
            uint32_t indices_size = (uint32_t)((uint8_t*)(s_data->mesh_indices_cur) - (uint8_t*)s_data->mesh_indices_base);
            s_data->mesh_index_buffer->set_data(s_data->mesh_indices_base, indices_size);
            RenderCommand::draw_indexed(indices_size / sizeof(uint32_t));

            s_data->mesh_vertices_cur = s_data->mesh_vertices_base;
            s_data->mesh_indices_cur = s_data->mesh_indices_base;
        }
    }

    void Renderer::draw_mesh(const std::string& name, const glm::mat4& transform, const Ref<Texture2D>& texture, const glm::vec4& color)
    {
        if (s_data->mesh_indices_cur - s_data->mesh_indices_base >= RendererData::max_indices || s_data->texture_slot_index >= RendererData::max_texture_slots) {
            flush();
            s_data->texture_slot_index = 1;
        }

        int texture_index = -1;
        if (texture) {
            for (uint32_t i = 0; i < s_data->texture_slot_index; i ++) {
                if (s_data->texture_slots[i] == texture) {
                    texture_index = i;
                    break;
                }
            }
            if (texture_index == -1) {
                texture_index = s_data->texture_slot_index;
                s_data->texture_slots[texture_index] = texture;
                texture->bind(1, texture_index);
                s_data->texture_slot_index ++;
            }
        }

        uint32_t vertices_count = s_data->mesh_vertices_cur - s_data->mesh_vertices_base;

        const Mesh& mesh = MeshManager::get_mesh(name);
        for (auto& [pos, texcoord] : mesh.vertices) {
            glm::vec4 position{pos.x, pos.y, pos.z, 1.0f};
            s_data->mesh_vertices_cur->position = transform * position;
            s_data->mesh_vertices_cur->color = color;
            s_data->mesh_vertices_cur->texcoord = texcoord;
            s_data->mesh_vertices_cur->texid = texture_index;
            s_data->mesh_vertices_cur ++;
        }
        for (auto& index : mesh.indices) {
            *s_data->mesh_indices_cur = vertices_count + index;
            s_data->mesh_indices_cur ++;
        }
    }

}