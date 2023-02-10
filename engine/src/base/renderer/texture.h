#pragma once

namespace Yogi {

    class Texture
    {
    public:
        virtual ~Texture() = default;
    
        virtual uint32_t get_width() const = 0;
        virtual uint32_t get_height() const = 0;

        virtual void set_data(void* data, size_t size) const = 0;
        virtual void bind(uint32_t slot = 0) const = 0;
    };

    class Texture2D : public Texture
    {
    public:
        ~Texture2D() = default;

        static Ref<Texture2D> create(const std::string& path);
        static Ref<Texture2D> create(uint32_t width, uint32_t height);
    };
    

}