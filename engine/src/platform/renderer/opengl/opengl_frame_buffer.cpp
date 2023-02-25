#include "platform/renderer/opengl/opengl_frame_buffer.h"
#include <glad/glad.h>

namespace Yogi {

    Ref<FrameBuffer> FrameBuffer::create(uint32_t width, uint32_t height, const std::vector<Ref<Texture2D>>& color_attachments)
    {
        return CreateRef<OpenGLFrameBuffer>(width, height, color_attachments);
    }

    OpenGLFrameBuffer::OpenGLFrameBuffer(uint32_t width, uint32_t height, const std::vector<Ref<Texture2D>>& color_attachments) : m_width(width), m_height(height)
    {
        YG_PROFILE_FUNCTION();

        YG_CORE_ASSERT(0 < color_attachments.size() && color_attachments.size() <= 4, "Wrong color attachments size!");

        glCreateFramebuffers(1, &m_renderer_id);
        glBindFramebuffer(GL_FRAMEBUFFER, m_renderer_id);

        for (const auto& attachment : color_attachments) {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + m_color_attachments_size, GL_TEXTURE_2D, attachment->get_renderer_id(), 0);
            m_color_attachments[m_color_attachments_size] = attachment;
            m_color_attachments_size ++;
        }
        GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
        glDrawBuffers(m_color_attachments_size, buffers);

        glCreateRenderbuffers(1, &m_render_buffer);
        glBindRenderbuffer(GL_RENDERBUFFER, m_render_buffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_render_buffer);

        YG_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    OpenGLFrameBuffer::~OpenGLFrameBuffer()
    {
        YG_PROFILE_FUNCTION();
        
        glDeleteFramebuffers(1, &m_renderer_id);
    }

    void OpenGLFrameBuffer::bind() const
    {
        YG_PROFILE_FUNCTION();

        glBindFramebuffer(GL_FRAMEBUFFER, m_renderer_id);
    }

    void OpenGLFrameBuffer::unbind() const
    {
        YG_PROFILE_FUNCTION();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFrameBuffer::resize(uint32_t width, uint32_t height)
    {
        YG_PROFILE_FUNCTION();

        m_width = width;
        m_height = height;

        glBindRenderbuffer(GL_RENDERBUFFER, m_render_buffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);
    }

    void OpenGLFrameBuffer::add_color_attachment(uint32_t index, Ref<Texture2D> attachment)
    {
        YG_PROFILE_FUNCTION();

        YG_CORE_ASSERT(index < 4 && !m_color_attachments[index], "Invalid attachment index!");

        glBindFramebuffer(GL_FRAMEBUFFER, m_renderer_id);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D, attachment->get_renderer_id(), 0);
        m_color_attachments[index] = attachment;
        m_color_attachments_size ++;

        GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
        glDrawBuffers(m_color_attachments_size, buffers);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFrameBuffer::remove_color_attachment(uint32_t index)
    {
        YG_PROFILE_FUNCTION();

        YG_CORE_ASSERT(index < 4 && m_color_attachments[index], "Invalid attachment index!");
        m_color_attachments[index] = nullptr;
    }

    Ref<Texture2D> OpenGLFrameBuffer::get_color_attachment(uint32_t index) const
    {
        YG_PROFILE_FUNCTION();

        YG_CORE_ASSERT(index < 4 && m_color_attachments[index], "Invalid attachment index!");
        return m_color_attachments[index];
    };

}