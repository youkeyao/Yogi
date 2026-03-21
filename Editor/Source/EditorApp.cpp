#include <Yogi.h>

#include "Core/EntryPoint.h"

#include "Reflect/ComponentManager.h"
#include "Reflect/SystemManager.h"

#include "Registry/AssetRegistry.h"

#include "Layers/ImGuiBeginLayer.h"
#include "Layers/ImGuiEndLayer.h"
#include "Layers/ViewportLayer.h"
#include "Layers/HierarchyLayer.h"
#include "Layers/MaterialEditorLayer.h"
#include "Layers/ContentBrowserLayer.h"
#include "Layers/RenderPassEditorLayer.h"

namespace Yogi
{

class EditorApp : public Application
{
public:
    EditorApp() : Application("Yogi Editor")
    {
        ComponentManager::Init();
        SystemManager::Init();

        AssetManager::PushAssetSource<FileSystemSource>(".");
        AssetRegistry::Init();
        AssetRegistry::Scan(".");

        PushLayer(Owner<ImGuiBeginLayer>::Create());
        PushLayer(Owner<ViewportLayer>::Create());
        PushLayer(Owner<HierarchyLayer>::Create());
        PushLayer(Owner<ContentBrowserLayer>::Create());
        PushLayer(Owner<MaterialEditorLayer>::Create());
        PushLayer(Owner<RenderPassEditorLayer>::Create());
        PushLayer(Owner<ImGuiEndLayer>::Create());
    }

    ~EditorApp()
    {
        ComponentManager::Clear();
        SystemManager::Clear();
        AssetRegistry::Clear();
    }
};

Application* CreateApplication() { return new EditorApp(); }

} // namespace Yogi