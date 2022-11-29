#include <engine.h>

class ExampleLayer : public hazel::Layer
{
public:
    ExampleLayer() : Layer("example")
    {

    }

    void on_update() override
    {
        if (hazel::Input::is_key_pressed(HZ_KEY_TAB))
            HZ_TRACE("Tab key is pressed!");
    }

    void on_event(hazel::Event& e) override
    {
        // HZ_TRACE("{0}", e);
    }
};

class Sandbox : public hazel::Application
{
public:
    Sandbox()
    {
        push_layer(new ExampleLayer());
        push_overlay(new hazel::ImGuiLayer());
    }

    ~Sandbox()
    {

    }
};

hazel::Application* hazel::create_application()
{
    return new Sandbox();
}