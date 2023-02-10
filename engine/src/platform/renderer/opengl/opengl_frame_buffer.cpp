#include "platform/renderer/opengl/opengl_frame_buffer.h"
#include <glad/glad.h>

namespace Yogi {

    Ref<FrameBuffer> FrameBuffer::create(const FrameBufferProps& props)
    {
        return CreateRef<OpenGLFrameBuffer>(props);
    }

	OpenGLFrameBuffer::OpenGLFrameBuffer(const FrameBufferProps& props) : m_width(props.width), m_height(props.height)
	{
		glCreateFramebuffers(1, &m_renderer_id);
		glBindFramebuffer(GL_FRAMEBUFFER, m_renderer_id);

        glCreateTextures(GL_TEXTURE_2D, 1, &m_color_attachment);
        glBindTexture(GL_TEXTURE_2D, m_color_attachment);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_color_attachment, 0);

        glCreateTextures(GL_TEXTURE_2D, 1, &m_depth_attachment);
        glBindTexture(GL_TEXTURE_2D, m_depth_attachment);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, m_width, m_height);

        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_depth_attachment, 0);

		YG_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	OpenGLFrameBuffer::~OpenGLFrameBuffer()
	{
		glDeleteFramebuffers(1, &m_renderer_id);
		glDeleteTextures(1, &m_color_attachment);
		glDeleteTextures(1, &m_depth_attachment);
	}

	void OpenGLFrameBuffer::bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_renderer_id);
		glViewport(0, 0, m_width, m_height);
	}

	void OpenGLFrameBuffer::unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

}