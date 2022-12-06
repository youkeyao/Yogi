#include "renderer/buffer.h"
#include "renderer/renderer.h"
#include "platform/opengl/opengl_buffer.h"

namespace hazel {

    VertexBuffer* VertexBuffer::create(float* vertices, uint32_t size)
    {
        switch (Renderer::get_api()) {
            case RendererAPI::None:
                HZ_CORE_ASSERT(false, "RendererAPI::None not supported!");
                return nullptr;

            case RendererAPI::OpenGL:
                return new OpenGLVertexBuffer(vertices, size);
        }

        HZ_CORE_ASSERT(false, "unknown RendererAPI!");
        return nullptr;
    }

    IndexBuffer* IndexBuffer::create(uint32_t* indices, uint32_t size)
    {
        switch (Renderer::get_api()) {
            case RendererAPI::None:
                HZ_CORE_ASSERT(false, "RendererAPI::None not supported!");
                return nullptr;

            case RendererAPI::OpenGL:
                return new OpenGLIndexBuffer(indices, size);
        }

        HZ_CORE_ASSERT(false, "unknown RendererAPI!");
        return nullptr;
    }

}