#include <engine.h>

class Sandbox : public hazel::Application
{
public:
    Sandbox()
    {
        HZ_INFO("hello!");
    }

    ~Sandbox()
    {

    }
};

hazel::Application* hazel::create_application()
{
    return new Sandbox();
}