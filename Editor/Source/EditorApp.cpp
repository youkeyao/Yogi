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
#include "Layers/PipelineEditorLayer.h"

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

        PushLayer(Handle<ImGuiBeginLayer>::Create());
        PushLayer(Handle<ViewportLayer>::Create());
        PushLayer(Handle<HierarchyLayer>::Create());
        PushLayer(Handle<MaterialEditorLayer>::Create());
        PushLayer(Handle<ContentBrowserLayer>::Create());
        PushLayer(Handle<RenderPassEditorLayer>::Create());
        PushLayer(Handle<PipelineEditorLayer>::Create());
        PushLayer(Handle<ImGuiEndLayer>::Create());
    }

    ~EditorApp()
    {
        ComponentManager::Clear();
        AssetRegistry::Clear();
    }
};

Application* CreateApplication() { return new EditorApp(); }

} // namespace Yogi