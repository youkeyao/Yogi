#include "renderer/vertex_array.h"
#include "renderer/renderer.h"
#include "platform/opengl/opengl_vertex_array.h"

namespace hazel {

    VertexArray* VertexArray::create()
    {
        switch (Renderer::get_api()) {
            case RendererAPI::API::None:
                HZ_CORE_ASSERT(false, "RendererAPI::None not supported");
                return nullptr;

            case RendererAPI::API::OpenGL:
                return new OpenGLVertexArray();
        }

        HZ_CORE_ASSERT(false, "unknown RendererAPI");
        return nullptr;
    }

}