#pragma once

#include "runtime/renderer/frame_buffer.h"

namespace Yogi {

    class OpenGLFrameBuffer : public FrameBuffer
    {
    public:
        OpenGLFrameBuffer(uint32_t width, uint32_t height, const std::vector<Ref<Texture2D>>& color_attachments, bool has_depth_attachment = true);
        ~OpenGLFrameBuffer();

        void bind() const override;
        void unbind() const override;

        void resize(uint32_t width, uint32_t height) override;
        uint32_t get_width() const override { return m_width; }
        uint32_t get_height() const override { return m_height; }

        const Ref<Texture2D>& get_color_attachment(uint32_t index) const override { return m_color_attachments[index]; }
    private:
        uint32_t m_renderer_id = 0;
        uint32_t m_width, m_height;

        std::vector<Ref<Texture2D>> m_color_attachments;
        uint32_t m_render_buffer;
    };

}