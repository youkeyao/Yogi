#include <engine.h>

class Sandbox : public hazel::Application
{
public:
    Sandbox()
    {

    }

    ~Sandbox()
    {

    }
};

hazel::Application* hazel::create_application()
{
    return new Sandbox();
}