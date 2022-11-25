#include <engine.h>

class Sandbox : public hazel::Application
{
public:
    Sandbox()
    {
        HZ_INFO("hello!");
        hazel::WindowResizeEvent e(1280, 720);
        HZ_TRACE(e);
    }

    ~Sandbox()
    {

    }
};

hazel::Application* hazel::create_application()
{
    return new Sandbox();
}