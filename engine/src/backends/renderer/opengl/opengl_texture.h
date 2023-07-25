#pragma once

#include "runtime/renderer/texture.h"

namespace Yogi {

    class OpenGLTexture2D : public Texture2D
    {
    public:
        OpenGLTexture2D(const std::string& name, const std::string& path);
        ~OpenGLTexture2D();

        uint32_t get_width() const override { return m_width; }
        uint32_t get_height() const override{ return m_height; }
        uint32_t get_renderer_id() const { return m_renderer_id; }
        void read_pixel(int32_t width, int32_t height, int32_t x, int32_t y, void* data) const override;

        void set_data(void* data, size_t size) override;

        void bind(uint32_t binding = 0, uint32_t slot = 0) const override;
    private:
        uint32_t m_width, m_height;
        uint32_t m_renderer_id;
        uint32_t m_internal_format;
        uint32_t m_data_format;
    };

    class OpenGLRenderTexture : public RenderTexture
    {
    public:
        OpenGLRenderTexture(const std::string& name, uint32_t width, uint32_t height, TextureFormat format = TextureFormat::RGBA8);
        ~OpenGLRenderTexture();

        uint32_t get_width() const override { return m_width; }
        uint32_t get_height() const override{ return m_height; }
        uint32_t get_renderer_id() const { return m_renderer_id; }
        void read_pixel(int32_t width, int32_t height, int32_t x, int32_t y, void* data) const override;

        void set_data(void* data, size_t size) override;

        void bind(uint32_t binding = 0, uint32_t slot = 0) const override;

        void resize(uint32_t width, uint32_t height) override;
    private:
        uint32_t m_width, m_height;
        uint32_t m_renderer_id;
        uint32_t m_internal_format;
        uint32_t m_data_format;
    };

}