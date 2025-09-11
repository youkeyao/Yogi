#include <Yogi.h>

#include "Core/EntryPoint.h"
#include "Sandbox2D.h"

class Sandbox : public Yogi::Application
{
public:
    Sandbox() { PushLayer(Yogi::Handle<Sandbox2D>::Create()); }

    ~Sandbox() {}
};

Yogi::Application* Yogi::CreateApplication()
{
    return new Sandbox();
}