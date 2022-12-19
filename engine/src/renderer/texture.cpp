#include "renderer/texture.h"
#include "renderer/renderer.h"
#include "platform/opengl/opengl_texture.h"

namespace hazel {

    Ref<Texture2D> Texture2D::create(uint32_t width, uint32_t height) {
        switch (Renderer::get_api()) {
            case RendererAPI::API::None:
                HZ_CORE_ASSERT(false, "RendererAPI::None not supported");
                return nullptr;

            case RendererAPI::API::OpenGL:
                return std::make_shared<OpenGLTexture2D>(width, height);
        }

        HZ_CORE_ASSERT(false, "unknown RendererAPI");
        return nullptr;
    }

    Ref<Texture2D> Texture2D::create(const std::string& path) {
        switch (Renderer::get_api()) {
            case RendererAPI::API::None:
                HZ_CORE_ASSERT(false, "RendererAPI::None not supported");
                return nullptr;

            case RendererAPI::API::OpenGL:
                return std::make_shared<OpenGLTexture2D>(path);
        }

        HZ_CORE_ASSERT(false, "unknown RendererAPI");
        return nullptr;
    }

}