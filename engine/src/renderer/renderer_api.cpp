#include "renderer/renderer_api.h"
#include "platform/opengl/opengl_renderer_api.h"

namespace hazel {

    RendererAPI::API RendererAPI::ms_api = RendererAPI::API::OpenGL;

}