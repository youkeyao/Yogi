#pragma once

#include "runtime/utility/md5.h"

namespace Yogi {

    enum class TextureFormat
    {
        None = 0,
        RGBA8,
        RED_INTEGER,
        ATTACHMENT
    };

    class Texture
    {
    public:
        virtual ~Texture() = default;
    
        virtual uint32_t get_width() const = 0;
        virtual uint32_t get_height() const = 0;

        virtual void read_pixel(int32_t width, int32_t height, int32_t x, int32_t y, void* data) const = 0;

        virtual void set_data(void* data, size_t size) = 0;
        virtual void bind(uint32_t binding = 0, uint32_t slot = 0) const = 0;

        void set_name(const std::string& name) { m_name = name; }
        const std::string& get_name() const { return m_name; }
    protected:
        std::string m_name;
    };

    class Texture2D : public Texture
    {
    public:
        ~Texture2D() = default;

        const std::string& get_digest() const { return m_digest; }

        static Ref<Texture2D> create(const std::string& name, const std::string& path);
    protected:
        std::string m_digest;
    };

    class RenderTexture : public Texture
    {
    public:
        ~RenderTexture() = default;

        virtual void resize(uint32_t width, uint32_t height) = 0;
        TextureFormat get_format() const { return m_format; }

        static Ref<RenderTexture> create(const std::string& name, uint32_t width, uint32_t height, TextureFormat format = TextureFormat::RGBA8);
    protected:
        TextureFormat m_format;
    };

}