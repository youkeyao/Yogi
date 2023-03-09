#pragma once

#include "runtime/renderer/frame_buffer.h"

namespace Yogi {

    class OpenGLFrameBuffer : public FrameBuffer
    {
    public:
        OpenGLFrameBuffer(uint32_t width, uint32_t height, const std::vector<Ref<Texture2D>>& color_attachments);
        ~OpenGLFrameBuffer();

        void bind() const override;
        void unbind() const override;

        void resize(uint32_t width, uint32_t height) override;
        uint32_t get_width() const override { return m_width; }
        uint32_t get_height() const override { return m_height; }

        void add_color_attachment(uint32_t index, const Ref<Texture2D>& attachment) override;
        void remove_color_attachment(uint32_t index) override;
        const Ref<Texture2D>& get_color_attachment(uint32_t index) const override;
    private:
        uint32_t m_renderer_id = 0;
        uint32_t m_width, m_height;

        Ref<Texture2D> m_color_attachments[4] = { nullptr, nullptr, nullptr, nullptr };
        uint32_t m_color_attachments_size = 0;
        uint32_t m_render_buffer;
    };

}