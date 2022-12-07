#include "renderer/renderer_command.h"
#include "platform/opengl/opengl_renderer_api.h"

namespace hazel {

    RendererAPI* RendererCommand::ms_renderer_api = new OpenGLRendererAPI;

}