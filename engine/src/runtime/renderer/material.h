#pragma once

#include "runtime/renderer/pipeline.h"
#include "runtime/renderer/texture.h"

namespace Yogi {

class Material
{
public:
    static Ref<Material> create(const std::string &name, const Ref<Pipeline> &pipeline);
    Material(const std::string &name, const Ref<Pipeline> &pipeline);
    ~Material();

    void set_pipeline(const Ref<Pipeline> &pipeline);

    void               set_name(const std::string &name) { m_name = name; }
    const std::string &get_name() const { return m_name; }

    const Ref<Pipeline> &get_pipeline() const { return m_pipeline; }
    int32_t              get_position_offset() const { return m_position_offset; }
    int32_t              get_normal_offset() const { return m_normal_offset; }
    int32_t              get_texcoord_offset() const { return m_texcoord_offset; }
    int32_t              get_entity_offset() const { return m_entity_offset; }

    void set_texture(uint32_t index, const Ref<Texture> &texture) { m_textures[index].second = texture; }
    const std::vector<std::pair<uint32_t, Ref<Texture>>> &get_textures() const { return m_textures; }
    uint8_t                                              *get_data() const { return m_data; }

private:
    std::string                                    m_name;
    Ref<Pipeline>                                  m_pipeline;
    int32_t                                        m_position_offset = -1;
    int32_t                                        m_normal_offset = -1;
    int32_t                                        m_texcoord_offset = -1;
    int32_t                                        m_entity_offset = -1;
    std::vector<std::pair<uint32_t, Ref<Texture>>> m_textures;
    uint8_t                                       *m_data = nullptr;
};

}  // namespace Yogi