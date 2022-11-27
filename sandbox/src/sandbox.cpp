#include <engine.h>

class ExampleLayer : public hazel::Layer
{
public:
    ExampleLayer() : Layer("example")
    {

    }

    void on_update() override
    {
        HZ_INFO("ExampleLayer::update");
    }

    void on_event(hazel::Event& e) override
    {
        HZ_TRACE("{0}", e);
    }
};

class Sandbox : public hazel::Application
{
public:
    Sandbox()
    {
        push_layer(new ExampleLayer());
    }

    ~Sandbox()
    {

    }
};

hazel::Application* hazel::create_application()
{
    return new Sandbox();
}