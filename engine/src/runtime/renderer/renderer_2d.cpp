#include "runtime/renderer/render_command.h"
#include "runtime/renderer/renderer_2d.h"
#include "runtime/renderer/shader.h"
#include "runtime/renderer/vertex_array.h"

namespace Yogi {

    struct QuadVertex
    {
        glm::vec3 position;
        glm::vec4 color;
        glm::vec2 texcoord;
        int texid;
        int entityid;
    };

    struct Renderer2DData
    {
        static const uint32_t max_quads = 10000;
        static const uint32_t max_vertices = max_quads * 4;
        static const uint32_t max_indices = max_quads * 6;
        static const uint32_t max_texture_slots = 32;

        Ref<VertexArray> quad_vertex_array;
        Ref<VertexBuffer> quad_vertex_buffer;
        Ref<Shader> texture_shader;

        uint32_t quad_index_count = 0;
        QuadVertex* quad_vertices_base = nullptr;
        QuadVertex* quad_vertices_cur = nullptr;

        std::array<Ref<Texture2D>, max_texture_slots> texture_slots;
        uint32_t texture_slot_index = 1;

        glm::vec4 quad_vertex_positions[4];

        Renderer2D::Statistics stats;
    };

    static Renderer2DData s_data;

    void Renderer2D::init()
    {
        YG_PROFILE_FUNCTION();
        
        s_data.quad_vertex_array = VertexArray::create();

        s_data.quad_vertex_buffer = VertexBuffer::create(nullptr, Renderer2DData::max_vertices * sizeof(QuadVertex), false);
        s_data.quad_vertex_buffer->set_layout({
            { ShaderDataType::Float3, "a_Position" },
            { ShaderDataType::Float4, "a_Color" },
            { ShaderDataType::Float2, "a_TexCoord" },
            { ShaderDataType::Int, "a_TexID" },
            { ShaderDataType::Int, "a_EntityID" },
        });
        s_data.quad_vertex_array->add_vertex_buffer(s_data.quad_vertex_buffer);

        s_data.quad_vertices_cur = s_data.quad_vertices_base = new QuadVertex[Renderer2DData::max_vertices];

        uint32_t* quad_indices = new uint32_t[Renderer2DData::max_indices];
        uint32_t offset = 0;
        for (uint32_t i = 0; i < Renderer2DData::max_indices; i += 6) {
            quad_indices[i] = offset + 0;
            quad_indices[i + 1] = offset + 1;
            quad_indices[i + 2] = offset + 2;
            quad_indices[i + 3] = offset + 2;
            quad_indices[i + 4] = offset + 3;
            quad_indices[i + 5] = offset + 0;
            offset += 4;
        }
        Ref<IndexBuffer> quad_ib = IndexBuffer::create(quad_indices, Renderer2DData::max_indices);
        s_data.quad_vertex_array->set_index_buffer(quad_ib);
        delete[] quad_indices;

        int32_t samplers[Renderer2DData::max_texture_slots];
        for (uint32_t i = 0; i < Renderer2DData::max_texture_slots; i++) {
            samplers[i] = i;
        }
        s_data.texture_shader = Yogi::Shader::create("Texture");
        s_data.texture_shader->bind();
        s_data.texture_shader->set_int_array("u_Textures", samplers, Renderer2DData::max_texture_slots);

        s_data.quad_vertex_positions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
		s_data.quad_vertex_positions[1] = {  0.5f, -0.5f, 0.0f, 1.0f };
		s_data.quad_vertex_positions[2] = {  0.5f,  0.5f, 0.0f, 1.0f };
		s_data.quad_vertex_positions[3] = { -0.5f,  0.5f, 0.0f, 1.0f };
    }

    void Renderer2D::shutdown()
    {
        YG_PROFILE_FUNCTION();

        delete[] s_data.quad_vertices_base;
    }

    void Renderer2D::set_projection_view_matrix(glm::mat4 projection_view_matrix)
    {
        YG_PROFILE_FUNCTION();

        s_data.texture_shader->bind();
        s_data.texture_shader->set_mat4("u_projection_view", projection_view_matrix);
    }

    void Renderer2D::flush()
    {
        YG_PROFILE_FUNCTION();

        if (s_data.quad_index_count) {
            s_data.texture_shader->bind();

            uint32_t vertices_size = (uint32_t)((uint8_t*)(s_data.quad_vertices_cur) - (uint8_t*)s_data.quad_vertices_base);
            s_data.quad_vertex_buffer->set_data(s_data.quad_vertices_base, vertices_size);
            RenderCommand::draw_indexed(s_data.quad_vertex_array, s_data.quad_index_count);
            s_data.stats.draw_calls++;

            s_data.quad_index_count = 0;
            s_data.quad_vertices_cur = s_data.quad_vertices_base;
        }
    }

    void Renderer2D::draw_quad(const glm::mat4& transform, const Ref<Texture2D>& texture, const std::pair<glm::vec2, glm::vec2>& texcoords, const glm::vec4& tint_color, uint32_t entity_id)
    {
        YG_PROFILE_FUNCTION();

        if (s_data.quad_index_count >= Renderer2DData::max_indices || s_data.texture_slot_index >= Renderer2DData::max_texture_slots) {
            flush();
            s_data.texture_slot_index = 1;
        }

        int texture_index = 0;
        if (texture) {
            for (uint32_t i = 1; i < s_data.texture_slot_index; i ++) {
                if (s_data.texture_slots[i] == texture) {
                    texture_index = i;
                    break;
                }
            }
            if (texture_index == 0.0f) {
                texture_index = s_data.texture_slot_index;
                s_data.texture_slots[s_data.texture_slot_index] = texture;
                texture->bind(s_data.texture_slot_index);
                s_data.texture_slot_index++;
            }
        }

        for (uint32_t i = 0; i < 4; i++) {
            switch (i) {
                case 0: s_data.quad_vertices_cur->texcoord = texcoords.first; break;
                case 1: s_data.quad_vertices_cur->texcoord = {texcoords.second.x, texcoords.first.y}; break;
                case 2: s_data.quad_vertices_cur->texcoord = texcoords.second; break;
                case 3: s_data.quad_vertices_cur->texcoord = {texcoords.first.x, texcoords.second.y}; break;
            }
            s_data.quad_vertices_cur->position = transform * s_data.quad_vertex_positions[i];
            s_data.quad_vertices_cur->color = tint_color;
            s_data.quad_vertices_cur->texid = texture_index;
            s_data.quad_vertices_cur->entityid = entity_id;
            s_data.quad_vertices_cur++;
		}

        s_data.quad_index_count += 6;
        s_data.stats.quad_count++;
    }

    void Renderer2D::reset_stats()
    {
        memset(&s_data.stats, 0, sizeof(Statistics));
    }

    Renderer2D::Statistics Renderer2D::get_stats()
    {
        return s_data.stats;
    }

}