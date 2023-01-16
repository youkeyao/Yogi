#include "renderer/vertex_array.h"
#include "renderer/renderer.h"
#include "platform/opengl/opengl_vertex_array.h"

namespace hazel {

    Ref<VertexArray> VertexArray::create()
    {
        switch (Renderer::get_api()) {
            case RendererAPI::API::None:
                HZ_CORE_ASSERT(false, "RendererAPI::None not supported");
                return nullptr;

            case RendererAPI::API::OpenGL:
                return std::make_shared<OpenGLVertexArray>();
        }

        HZ_CORE_ASSERT(false, "unknown RendererAPI");
        return nullptr;
    }

}