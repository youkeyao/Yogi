#pragma once

#include "base/renderer/texture.h"

namespace Yogi {

    class FrameBuffer
    {
    public:
        virtual ~FrameBuffer() = default;

        virtual void bind() const = 0;
        virtual void unbind() const = 0;

        virtual void resize(uint32_t width, uint32_t height) = 0;
        virtual uint32_t get_width() const = 0;
        virtual uint32_t get_height() const = 0;

        virtual void add_color_attachment(uint32_t index, Ref<Texture2D> attachment) = 0;
        virtual void remove_color_attachment(uint32_t index) = 0;
        virtual Ref<Texture2D> get_color_attachment(uint32_t index) const = 0;

        static Ref<FrameBuffer> create(uint32_t width, uint32_t height, const std::vector<Ref<Texture2D>>& color_attachments);
    };

}