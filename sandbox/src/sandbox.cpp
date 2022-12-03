#include <engine.h>
#include <imgui.h>

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

    void on_imgui_render()
    {
        ImGui::Begin("Test");
        ImGui::Text("hello world");
        ImGui::End();
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