#pragma once

#include "runtime/renderer/texture.h"
#include <glad/glad.h>

namespace Yogi {

    class OpenGLTexture2D : public Texture2D
    {
    public:
        OpenGLTexture2D(uint32_t width, uint32_t height, TextureFormat format = TextureFormat::RGBA8);
        OpenGLTexture2D(const std::string& path);
        ~OpenGLTexture2D();

        uint32_t get_width() const override { return m_width; }
        uint32_t get_height() const override{ return m_height; }
        void* get_renderer_id() const override { return (void*)m_renderer_id; }
        void read_pixel(int32_t x, int32_t y, void* data) const override;

        void set_data(void* data, size_t size) override;

        void bind(uint32_t binding = 0, uint32_t slot = 0) const override;
    private:
        uint32_t m_width, m_height;
        uint32_t m_renderer_id;
        GLenum m_internal_format;
        GLenum m_data_format;
    };

}