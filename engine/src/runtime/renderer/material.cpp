#include "runtime/renderer/material.h"

namespace Yogi {

    Ref<Material> Material::create(const std::string& name, const Ref<Pipeline>& pipeline)
    {
        return CreateRef<Material>(name, pipeline);
    }

    Material::Material(const std::string& name, const Ref<Pipeline>& pipeline) : m_pipeline(pipeline), m_name(name)
    {
        PipelineLayout vertex_layout = m_pipeline->get_vertex_layout();
        m_data = new uint8_t[vertex_layout.get_stride()];
        for (auto& element : vertex_layout.get_elements()) {
            if (element.name == "a_Position") {
                m_position_offset = element.offset;
            }
            else if (element.name == "a_TexCoord") {
                m_texcoord_offset = element.offset;
            }
            else if (element.name == "a_EntityID") {
                m_entity_offset = element.offset;
            }
            else if (element.name.substr(0, 4) == "TEX_") {
                m_textures.push_back({element.offset, nullptr});
            }
        }
    }

    Material::~Material()
    {
        delete m_data;
    }

    void Material::set_pipeline(const Ref<Pipeline>& pipeline)
    {
        delete m_data;
        m_textures.clear();
        m_pipeline = pipeline;
        PipelineLayout vertex_layout = m_pipeline->get_vertex_layout();
        m_data = new uint8_t[vertex_layout.get_stride()];
        for (auto& element : vertex_layout.get_elements()) {
            if (element.name == "a_Position") {
                m_position_offset = element.offset;
            }
            else if (element.name == "a_TexCoord") {
                m_texcoord_offset = element.offset;
            }
            else if (element.name == "a_EntityID") {
                m_entity_offset = element.offset;
            }
            else if (element.name.substr(0, 4) == "TEX_") {
                m_textures.push_back({element.offset, nullptr});
            }
        }
    }

}