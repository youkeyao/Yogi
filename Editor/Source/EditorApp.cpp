#include <Yogi.h>

#include "Core/EntryPoint.h"
#include "Layers/ImGuiBeginLayer.h"
#include "Layers/ImGuiEndLayer.h"
#include "Layers/ViewportLayer.h"
#include "Layers/HierarchyLayer.h"

namespace Yogi
{

class EditorApp : public Application
{
public:
    EditorApp() : Application("Yogi Editor"), m_world(Handle<World>::Create()), m_selectedEntity(Entity::Null())
    {
        PushLayer(Handle<ImGuiBeginLayer>::Create());
        PushLayer(Handle<ViewportLayer>::Create(m_world, m_selectedEntity));
        PushLayer(Handle<HierarchyLayer>::Create(m_world, m_selectedEntity));
        PushLayer(Handle<ImGuiEndLayer>::Create());
    }

    ~EditorApp() {}

private:
    Handle<World> m_world;
    Entity        m_selectedEntity;
};

Application* CreateApplication() { return new EditorApp(); }

} // namespace Yogi