#include "backends/renderer/opengl/opengl_texture.h"

#include <glad/glad.h>
#include <stb_image.h>

namespace Yogi {

Ref<RenderTexture> RenderTexture::create(const std::string &name, uint32_t width, uint32_t height, TextureFormat format)
{
    return CreateRef<OpenGLRenderTexture>(name, width, height, format);
}

Ref<Texture2D> Texture2D::create(const std::string &name, const std::string &path)
{
    return CreateRef<OpenGLTexture2D>(name, path);
}

OpenGLTexture2D::OpenGLTexture2D(const std::string &name, const std::string &path)
{
    m_name = name;
    stbi_set_flip_vertically_on_load(1);

    int      width, height, channels;
    stbi_uc *data = nullptr;
    {
        YG_PROFILE_SCOPE("stbi_load - OpenGLTexture2D::OpenGLTexture2D(const std::string&)");
        data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    }
    YG_CORE_ASSERT(data, "failed to load image");
    m_width = width;
    m_height = height;

    if (channels == 4) {
        m_internal_format = GL_RGBA8;
        m_data_format = GL_RGBA;
    } else if (channels == 3) {
        m_internal_format = GL_RGBA8;
        m_data_format = GL_RGBA;
        stbi_uc *rgbaData = (stbi_uc *)malloc(m_width * m_height * 4);
        for (int i = 0; i < m_width * m_height; i++) {
            rgbaData[i * 4 + 0] = data[i * 3 + 0];  // R
            rgbaData[i * 4 + 1] = data[i * 3 + 1];  // G
            rgbaData[i * 4 + 2] = data[i * 3 + 2];  // B
            rgbaData[i * 4 + 3] = 255;              // A
        }
        stbi_image_free(data);
        data = rgbaData;
    }

    YG_CORE_ASSERT(m_internal_format & m_data_format, "format not supported");

    glCreateTextures(GL_TEXTURE_2D, 1, &m_renderer_id);
    glTextureStorage2D(m_renderer_id, 1, m_internal_format, m_width, m_height);

    glTextureParameteri(m_renderer_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_renderer_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTextureParameteri(m_renderer_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_renderer_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_renderer_id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glTextureSubImage2D(m_renderer_id, 0, 0, 0, m_width, m_height, m_data_format, GL_UNSIGNED_BYTE, data);
    m_digest = MD5(std::string{ data, data + m_width * m_height * 4 }).toStr();

    stbi_image_free(data);
}

OpenGLTexture2D::~OpenGLTexture2D()
{
    glDeleteTextures(1, &m_renderer_id);
}

void OpenGLTexture2D::read_pixel(int32_t x, int32_t y, void *data) const
{
    uint32_t bpp = 0;
    GLenum   type = GL_UNSIGNED_BYTE;
    if (m_data_format == GL_RGBA) {
        bpp = 4;
    } else if (m_data_format == GL_RGB) {
        bpp = 3;
    } else if (m_data_format == GL_RED_INTEGER) {
        type = GL_INT;
        bpp = 4;
    }
    char *all_pixels = new char[m_width * m_height * bpp];
    glGetTextureImage(m_renderer_id, 0, m_data_format, type, m_width * m_height * bpp, all_pixels);
    memcpy(data, all_pixels + x * bpp + y * m_width * bpp, bpp);
    delete[] all_pixels;
}

void OpenGLTexture2D::blit(const Ref<Texture> &dst)
{
    GLuint dst_renderer_id;
    if (std::dynamic_pointer_cast<OpenGLTexture2D>(dst)) {
        OpenGLTexture2D *dst_texture = (OpenGLTexture2D *)dst.get();
        dst_renderer_id = dst_texture->get_renderer_id();
    } else if (std::dynamic_pointer_cast<OpenGLRenderTexture>(dst)) {
        OpenGLRenderTexture *dst_texture = (OpenGLRenderTexture *)dst.get();
        dst_renderer_id = dst_texture->get_renderer_id();
    } else {
        YG_CORE_ERROR("Blit invalid texture type!");
        return;
    }
    glCopyImageSubData(
        m_renderer_id, GL_TEXTURE_2D, 0, 0, 0, 0, dst_renderer_id, GL_TEXTURE_2D, 0, 0, 0, 0, m_width, m_height, 1);
}

void OpenGLTexture2D::set_data(void *data, size_t size)
{
    uint32_t bpp = 0;
    GLenum   type = GL_UNSIGNED_BYTE;
    if (m_data_format == GL_RGBA) {
        bpp = 4;
    } else if (m_data_format == GL_RGB) {
        bpp = 3;
    } else if (m_data_format == GL_RED_INTEGER) {
        type = GL_INT;
        bpp = 4;
    }
    YG_CORE_ASSERT(size == m_width * m_height * bpp, "Data must be entire texture!");

    glTextureSubImage2D(m_renderer_id, 0, 0, 0, m_width, m_height, m_data_format, type, data);
};

void OpenGLTexture2D::bind(uint32_t binding, uint32_t slot) const
{
    glBindTextureUnit(slot + 1, m_renderer_id);
}

//-------------------------------------------------------------------------------------

OpenGLRenderTexture::OpenGLRenderTexture(const std::string &name, uint32_t width, uint32_t height, TextureFormat format)
    : m_width(width), m_height(height)
{
    m_name = name;
    m_format = format;
    if (format == TextureFormat::RGBA8) {
        m_internal_format = GL_RGBA8;
        m_data_format = GL_RGBA;
    } else if (format == TextureFormat::RED_INTEGER) {
        m_internal_format = GL_R32I;
        m_data_format = GL_RED_INTEGER;
    } else if (format == TextureFormat::ATTACHMENT) {
        m_internal_format = GL_RGBA8;
        m_data_format = GL_RGBA;
    } else {
        YG_CORE_ERROR("Invalid texture format!");
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &m_renderer_id);
    glBindTexture(GL_TEXTURE_2D, m_renderer_id);
    glTexImage2D(GL_TEXTURE_2D, 0, m_internal_format, width, height, 0, m_data_format, GL_UNSIGNED_BYTE, NULL);

    glTextureParameteri(m_renderer_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_renderer_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTextureParameteri(m_renderer_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_renderer_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_renderer_id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
}

OpenGLRenderTexture::~OpenGLRenderTexture()
{
    glDeleteTextures(1, &m_renderer_id);
}

void OpenGLRenderTexture::read_pixel(int32_t x, int32_t y, void *data) const
{
    uint32_t bpp = 0;
    GLenum   type = GL_UNSIGNED_BYTE;
    if (m_data_format == GL_RGBA) {
        bpp = 4;
    } else if (m_data_format == GL_RGB) {
        bpp = 3;
    } else if (m_data_format == GL_RED_INTEGER) {
        type = GL_INT;
        bpp = 4;
    }
    char *all_pixels = new char[m_width * m_height * bpp];
    glGetTextureImage(m_renderer_id, 0, m_data_format, type, m_width * m_height * bpp, all_pixels);
    memcpy(data, all_pixels + x * bpp + y * m_width * bpp, bpp);
    delete[] all_pixels;
}

void OpenGLRenderTexture::blit(const Ref<Texture> &dst)
{
    GLuint dst_renderer_id;
    if (std::dynamic_pointer_cast<OpenGLTexture2D>(dst)) {
        OpenGLTexture2D *dst_texture = (OpenGLTexture2D *)dst.get();
        dst_renderer_id = dst_texture->get_renderer_id();
    } else if (std::dynamic_pointer_cast<OpenGLRenderTexture>(dst)) {
        OpenGLRenderTexture *dst_texture = (OpenGLRenderTexture *)dst.get();
        dst_renderer_id = dst_texture->m_renderer_id;
    } else {
        YG_CORE_ERROR("Blit invalid texture type!");
        return;
    }
    glCopyImageSubData(
        m_renderer_id, GL_TEXTURE_2D, 0, 0, 0, 0, dst_renderer_id, GL_TEXTURE_2D, 0, 0, 0, 0, m_width, m_height, 1);
}

void OpenGLRenderTexture::set_data(void *data, size_t size)
{
    uint32_t bpp = 0;
    GLenum   type = GL_UNSIGNED_BYTE;
    if (m_data_format == GL_RGBA) {
        bpp = 4;
    } else if (m_data_format == GL_RGB) {
        bpp = 3;
    } else if (m_data_format == GL_RED_INTEGER) {
        type = GL_INT;
        bpp = 4;
    }
    YG_CORE_ASSERT(size == m_width * m_height * bpp, "Data must be entire texture!");

    glTextureSubImage2D(m_renderer_id, 0, 0, 0, m_width, m_height, m_data_format, type, data);
};

void OpenGLRenderTexture::bind(uint32_t binding, uint32_t slot) const
{
    glBindTextureUnit(binding + slot, m_renderer_id);
}

void OpenGLRenderTexture::resize(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;

    glBindTexture(GL_TEXTURE_2D, m_renderer_id);
    glTexImage2D(GL_TEXTURE_2D, 0, m_internal_format, width, height, 0, m_data_format, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);
}

}  // namespace Yogi