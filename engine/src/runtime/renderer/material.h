#pragma once

#include "runtime/renderer/pipeline.h"
#include "runtime/renderer/texture.h"

namespace Yogi {

    class Material
    {
    public:
        Material(const Ref<Pipeline>& pipeline);
        ~Material();
        const Ref<Pipeline>& get_pipeline() const { return m_pipeline; }
        int32_t get_position_offset() const { return m_position_offset; }
        int32_t get_texcoord_offset() const { return m_texcoord_offset; }
        void set_texture(uint32_t index, const Ref<Texture2D>& texture) { m_textures[index].second = texture; }
        const std::vector<std::pair<uint32_t, Ref<Texture2D>>>& get_textures() const { return m_textures; }
        uint8_t* get_data() const { return m_data; }
    private:
        uint8_t* m_data = nullptr;
        int32_t m_position_offset = -1;
        int32_t m_texcoord_offset = -1;
        std::vector<std::pair<uint32_t, Ref<Texture2D>>> m_textures;
        Ref<Pipeline> m_pipeline;
    };

}