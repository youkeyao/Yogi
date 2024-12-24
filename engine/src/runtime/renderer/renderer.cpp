#include "runtime/renderer/renderer.h"

#include "runtime/core/application.h"
#include "runtime/renderer/buffer.h"
#include "runtime/renderer/render_command.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Yogi {

struct RendererData
{
    static const uint32_t max_triangles = 100000;
    static const uint32_t max_vertices = max_triangles * 3;
    static const uint32_t max_vertices_size = max_vertices * 40;
    static const uint32_t max_indices = max_triangles * 3;
    static const uint32_t max_texture_slots = 32;

    Ref<VertexBuffer>  mesh_vertex_buffer;
    Ref<IndexBuffer>   mesh_index_buffer;
    Ref<UniformBuffer> scene_uniform_buffer;

    Ref<Pipeline> now_pipeline;
    uint8_t     **now_vertices_base;
    uint8_t     **now_vertices_cur;
    uint32_t    **now_indices_base;
    uint32_t    **now_indices_cur;

    std::map<Ref<Pipeline>, uint8_t *> mesh_vertices_bases;
    std::map<Ref<Pipeline>, uint8_t *> mesh_vertices_curs;

    std::map<Ref<Pipeline>, uint32_t *> mesh_indices_bases;
    std::map<Ref<Pipeline>, uint32_t *> mesh_indices_curs;

    uint32_t                                    texture_init_slot_index = 0;
    uint32_t                                    texture_slot_index = 0;
    std::array<Ref<Texture>, max_texture_slots> texture_slots;

    Renderer::SceneData scene_data;

    Renderer::Statistics statistics;
};

static RendererData *s_data;

void Renderer::init()
{
    YG_PROFILE_FUNCTION();

    s_data = new RendererData();

    s_data->mesh_vertex_buffer = VertexBuffer::create(nullptr, RendererData::max_vertices_size, false);
    s_data->mesh_vertex_buffer->bind();
    s_data->mesh_index_buffer = IndexBuffer::create(nullptr, RendererData::max_indices, false);
    s_data->mesh_index_buffer->bind();
    s_data->scene_uniform_buffer = UniformBuffer::create(sizeof(Renderer::SceneData));
}

void Renderer::shutdown()
{
    YG_PROFILE_FUNCTION();

    for (auto &[pipeline_name, vertices_base] : s_data->mesh_vertices_bases) {
        delete[] vertices_base;
    }
    for (auto &[pipeline_name, indices_base] : s_data->mesh_indices_bases) {
        delete[] indices_base;
    }

    delete s_data;
}

void Renderer::set_pipeline(const Ref<Pipeline> &pipeline)
{
    pipeline->bind();
    s_data->scene_uniform_buffer->bind(0);
    s_data->scene_uniform_buffer->set_data(&s_data->scene_data, sizeof(Renderer::SceneData));

    s_data->now_pipeline = pipeline;
    s_data->now_indices_base = &(s_data->mesh_indices_bases[pipeline]);
    s_data->now_indices_cur = &(s_data->mesh_indices_curs[pipeline]);
    s_data->now_vertices_base = &(s_data->mesh_vertices_bases[pipeline]);
    s_data->now_vertices_cur = &(s_data->mesh_vertices_curs[pipeline]);
}

void Renderer::set_projection_view_matrix(glm::mat4 projection_view_matrix)
{
    s_data->scene_data.projection_view_matrix = projection_view_matrix;
}
void Renderer::set_view_pos(glm::vec3 view_pos)
{
    s_data->scene_data.view_pos = view_pos;
}
void Renderer::reset_lights()
{
    s_data->scene_data.directional_light_color = glm::vec4(0.0f);
    s_data->scene_data.spot_light_num = 0;
    s_data->scene_data.point_light_num = 0;
}
void Renderer::set_directional_light(glm::vec4 color, glm::vec3 direction, glm::mat4 light_space_matrix)
{
    s_data->scene_data.directional_light_color = color;
    s_data->scene_data.directional_light_direction = direction;
    s_data->scene_data.directional_light_space_matrix = light_space_matrix;
}
bool Renderer::add_spot_light(SceneData::SpotLight light)
{
    if (s_data->scene_data.spot_light_num >= 4)
        return false;
    s_data->scene_data.spot_lights[s_data->scene_data.spot_light_num++] = light;
    return true;
}
bool Renderer::add_point_light(SceneData::PointLight light)
{
    if (s_data->scene_data.point_light_num >= 4)
        return false;
    s_data->scene_data.point_lights[s_data->scene_data.point_light_num++] = light;
    return true;
}

void Renderer::reset_stats()
{
    s_data->statistics = Statistics();
}

Renderer::Statistics Renderer::get_stats()
{
    return s_data->statistics;
}

void Renderer::set_texture_init_slot(uint32_t index)
{
    s_data->texture_init_slot_index = index;
}

void Renderer::set_current_texture_slot(uint32_t index)
{
    s_data->texture_slot_index = index;
}

uint32_t Renderer::get_current_texture_slot()
{
    return s_data->texture_slot_index;
}

int32_t Renderer::add_texture(const Ref<Texture> &texture)
{
    int32_t texture_index = -1;
    if (texture) {
        for (uint32_t i = 0; i < s_data->texture_slot_index; i++) {
            if (s_data->texture_slots[i] == texture) {
                texture_index = i;
                return texture_index;
            }
        }
        texture_index = s_data->texture_slot_index;
        s_data->texture_slots[texture_index] = texture;
        s_data->texture_slot_index++;
    }
    return texture_index;
}

void Renderer::flush_pipeline(const Ref<Pipeline> &pipeline)
{
    YG_PROFILE_FUNCTION();

    if (pipeline && s_data->mesh_indices_bases[pipeline] != s_data->mesh_indices_curs[pipeline]) {
        set_pipeline(pipeline);

        for (int32_t i = 0; i < s_data->texture_slot_index; i++) {
            auto &texture = s_data->texture_slots[i];
            if (texture)
                texture->bind(1, i);
        }

        uint32_t vertices_size = (uint32_t)(*(s_data->now_vertices_cur) - *(s_data->now_vertices_base));
        s_data->mesh_vertex_buffer->set_data(*(s_data->now_vertices_base), vertices_size);
        uint32_t indices_size = (uint32_t)((uint8_t *)*(s_data->now_indices_cur) - (uint8_t *)*(s_data->now_indices_base));
        s_data->mesh_index_buffer->set_data(*(s_data->now_indices_base), indices_size);
        RenderCommand::draw_indexed(indices_size / sizeof(uint32_t));

        *(s_data->now_vertices_cur) = *(s_data->now_vertices_base);
        *(s_data->now_indices_cur) = *(s_data->now_indices_base);

        uint32_t vertex_stride = pipeline->get_vertex_layout().get_stride();
        s_data->statistics.draw_calls++;
        s_data->statistics.vertices_count += vertices_size / vertex_stride;
        s_data->statistics.indices_count += indices_size / sizeof(uint32_t);
    }
}

void Renderer::flush()
{
    YG_PROFILE_FUNCTION();

    for (auto &[pipeline, vertices] : s_data->mesh_vertices_bases) {
        flush_pipeline(pipeline);
    }
}

void Renderer::draw_mesh(const Ref<Mesh> &mesh, const Ref<Material> &material, const glm::mat4 &transform, uint32_t entity_id)
{
    const std::vector<std::pair<uint32_t, Ref<Texture>>> &textures = material->get_textures();
    const Ref<Pipeline>                                  &pipeline = material->get_pipeline();
    if (s_data->now_pipeline != pipeline) {
        if (s_data->mesh_vertices_bases.find(pipeline) == s_data->mesh_vertices_bases.end()) {
            s_data->mesh_vertices_curs[pipeline] = s_data->mesh_vertices_bases[pipeline] =
                new uint8_t[RendererData::max_vertices_size];
            s_data->mesh_indices_curs[pipeline] = s_data->mesh_indices_bases[pipeline] =
                new uint32_t[RendererData::max_indices];
        }
        s_data->now_pipeline = pipeline;
        s_data->now_vertices_base = &(s_data->mesh_vertices_bases[pipeline]);
        s_data->now_vertices_cur = &(s_data->mesh_vertices_curs[pipeline]);
        s_data->now_indices_base = &(s_data->mesh_indices_bases[pipeline]);
        s_data->now_indices_cur = &(s_data->mesh_indices_curs[pipeline]);
    }

    auto &vertices_cur = *s_data->now_vertices_cur;
    auto &indices_cur = *s_data->now_indices_cur;
    auto &vertices_base = *s_data->now_vertices_base;
    auto &indices_base = *s_data->now_indices_base;

    uint32_t vertex_stride = pipeline->get_vertex_layout().get_stride();

    if (s_data->texture_slot_index + textures.size() >= RendererData::max_texture_slots) {
        flush();
        s_data->texture_slot_index = s_data->texture_init_slot_index;
    }
    if (vertices_cur - vertices_base + vertex_stride * mesh->vertices.size() >= RendererData::max_vertices_size ||
        indices_cur - indices_base + mesh->indices.size() >= RendererData::max_indices) {
        flush_pipeline(pipeline);
    }

    for (auto &[texture_offset, texture] : textures) {
        int32_t texture_index = add_texture(texture);
        memcpy(material->get_data() + texture_offset, &texture_index, 4);
    }

    uint32_t vertices_count = (vertices_cur - vertices_base) / vertex_stride;

    for (auto &vertex : mesh->vertices) {
        memcpy(vertices_cur, material->get_data(), vertex_stride);
        int32_t position_offset = material->get_position_offset();
        int32_t normal_offset = material->get_normal_offset();
        int32_t texcoord_offset = material->get_texcoord_offset();
        int32_t entity_offset = material->get_entity_offset();
        if (position_offset >= 0) {
            glm::vec4 position{ vertex.position.x, vertex.position.y, vertex.position.z, 1.0f };
            position = transform * position;
            memcpy(vertices_cur + position_offset, &position, 12);
        }
        if (normal_offset >= 0) {
            glm::vec4 normal{ vertex.normal.x, vertex.normal.y, vertex.normal.z, 0.0f };
            normal = transform * normal;
            memcpy(vertices_cur + normal_offset, &normal, 12);
        }
        if (texcoord_offset >= 0) {
            memcpy(vertices_cur + texcoord_offset, &vertex.texcoord, 8);
        }
        if (entity_offset >= 0) {
            memcpy(vertices_cur + entity_offset, &entity_id, 4);
        }
        vertices_cur += vertex_stride;
    }
    for (auto &index : mesh->indices) {
        *indices_cur = vertices_count + index;
        indices_cur++;
    }
}

}  // namespace Yogi