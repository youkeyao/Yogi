#include <engine.h>
#include "base/core/entrypoint.h"

#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

#include "sandbox2d.h"

class Sandbox : public hazel::Application
{
public:
    Sandbox()
    {
        push_layer(new Sandbox2D());
    }

    ~Sandbox()
    {

    }
};

hazel::Application* hazel::create_application()
{
    return new Sandbox();
}