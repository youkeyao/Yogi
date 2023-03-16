#pragma once

namespace Yogi {

    enum class TextureFormat
    {
        None = 0,
        RGBA8,
        RED_INTEGER
    };

    class Texture
    {
    public:
        virtual ~Texture() = default;
    
        virtual uint32_t get_width() const = 0;
        virtual uint32_t get_height() const = 0;

        virtual uint32_t get_renderer_id() const = 0;
        virtual void read_pixel(int32_t x, int32_t y, void* data) const = 0;

        virtual void set_data(void* data, size_t size) = 0;
        virtual void bind(uint32_t binding = 0, uint32_t slot = 0) const = 0;
    };

    class Texture2D : public Texture
    {
    public:
        ~Texture2D() = default;

        static Ref<Texture2D> create(const std::string& path);
        static Ref<Texture2D> create(uint32_t width, uint32_t height, TextureFormat format = TextureFormat::RGBA8);
    };
    

}