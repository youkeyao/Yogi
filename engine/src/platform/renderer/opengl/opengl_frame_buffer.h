#pragma once

#include "base/renderer/frame_buffer.h"

namespace Yogi {

    class OpenGLFrameBuffer : public FrameBuffer
    {
    public:
		OpenGLFrameBuffer(const FrameBufferProps& props);
		~OpenGLFrameBuffer();

		void bind() override;
		void unbind() override;

        uint32_t get_color_attachment() const override { return m_color_attachment; }
	private:
		uint32_t m_renderer_id = 0;
		uint32_t m_width, m_height;

		uint32_t m_color_attachment;
		uint32_t m_depth_attachment;
    };

}