#pragma once
#include "base/renderer/texture.h"
#include <glad/glad.h>

namespace hazel {

    class OpenGLTexture2D : public Texture2D
    {
    public:
        OpenGLTexture2D(uint32_t width, uint32_t height);
        OpenGLTexture2D(const std::string& path);
        virtual ~OpenGLTexture2D();

        uint32_t get_width() const override { return m_width; }
        uint32_t get_height() const override{ return m_height; }

        void set_data(void* data, size_t size) const override;

        void bind(uint32_t slot = 0) const override;

    private:
        std::string m_path;
        uint32_t m_width, m_height;
        uint32_t m_renderer_id;
        GLenum m_internal_format;
        GLenum m_data_format;
    };

}