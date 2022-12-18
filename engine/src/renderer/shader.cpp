#include "renderer/shader.h"
#include "renderer/renderer.h"
#include "platform/opengl/opengl_shader.h"

namespace hazel {

    Shader* Shader::create(const std::string& vertex_source, const std::string& fragment_source)
    {
        switch (Renderer::get_api()) {
            case RendererAPI::API::None:
                HZ_CORE_ASSERT(false, "RendererAPI::None not supported");
                return nullptr;

            case RendererAPI::API::OpenGL:
                return new OpenGLShader(vertex_source, fragment_source);
        }

        HZ_CORE_ASSERT(false, "unknown RendererAPI");
        return nullptr;
    }

}