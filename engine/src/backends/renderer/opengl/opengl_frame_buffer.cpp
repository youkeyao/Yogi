#include "backends/renderer/opengl/opengl_frame_buffer.h"
#include "backends/renderer/opengl/opengl_texture.h"
#include <glad/glad.h>

namespace Yogi {

    Ref<FrameBuffer> FrameBuffer::create(uint32_t width, uint32_t height, const std::vector<Ref<Texture2D>>& color_attachments, bool has_depth_attachment)
    {
        return CreateRef<OpenGLFrameBuffer>(width, height, color_attachments, has_depth_attachment);
    }

    OpenGLFrameBuffer::OpenGLFrameBuffer(uint32_t width, uint32_t height, const std::vector<Ref<Texture2D>>& color_attachments, bool has_depth_attachment)
    : m_width(width), m_height(height), m_color_attachments(color_attachments)
    {
        YG_CORE_ASSERT(0 < color_attachments.size() && color_attachments.size() <= 4, "Wrong color attachments size!");

        glCreateFramebuffers(1, &m_renderer_id);
        glBindFramebuffer(GL_FRAMEBUFFER, m_renderer_id);

        std::vector<GLenum> buffers(color_attachments.size());
        for (int32_t i = 0; i < color_attachments.size(); i ++) {
            auto& attachment = color_attachments[i];
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, ((OpenGLTexture2D*)attachment.get())->get_renderer_id(), 0);
            buffers[i] = GL_COLOR_ATTACHMENT0 + i;
        }
        glDrawBuffers(color_attachments.size(), buffers.data());

        if (has_depth_attachment) {
            glCreateRenderbuffers(1, &m_render_buffer);
            glBindRenderbuffer(GL_RENDERBUFFER, m_render_buffer);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_render_buffer);
        }

        YG_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    OpenGLFrameBuffer::~OpenGLFrameBuffer()
    {        
        glDeleteFramebuffers(1, &m_renderer_id);
    }

    void OpenGLFrameBuffer::bind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_renderer_id);
    }

    void OpenGLFrameBuffer::unbind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFrameBuffer::resize(uint32_t width, uint32_t height)
    {
        m_width = width;
        m_height = height;

        glBindRenderbuffer(GL_RENDERBUFFER, m_render_buffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);
    }

}