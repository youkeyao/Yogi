#include <engine.h>
#include "runtime/core/entrypoint.h"

#include "sandbox2d.h"

class Sandbox : public Yogi::Application
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

Yogi::Application* Yogi::create_application()
{
    return new Sandbox();
}