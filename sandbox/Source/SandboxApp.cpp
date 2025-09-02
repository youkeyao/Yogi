#include <Yogi.h>

#include "Core/EntryPoint.h"
#include "Sandbox2D.h"

class Sandbox : public Yogi::Application
{
public:
    Sandbox() { PushLayer(Yogi::CreateScope<Sandbox2D>()); }

    ~Sandbox() {}
};

Yogi::Application* Yogi::CreateApplication()
{
    return new Sandbox();
}