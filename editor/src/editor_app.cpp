#include <engine.h>
#include "base/core/entrypoint.h"

#include "editor_layer.h"
#include "imgui_layer.h"

namespace Yogi {

    class Editor : public Application
    {
    public:
        Editor() : Application("Yogi Editor")
        {
            push_layer(new ImguiLayer());
            push_layer(new EditorLayer());
        }

        ~Editor()
        {

        }
    };

    Application* create_application()
    {
        return new Editor();
    }

}