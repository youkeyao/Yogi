#pragma once

#include <engine.h>
#include <imgui.h>

namespace Yogi {

    class ImguiSetting
    {
    public:
        static void init();
        static void shutdown();
        static void imgui_begin();
        static void imgui_end();
        static ImTextureID get_texture_id(const Ref<Texture2D>& t);
        static void imgui_on_event(Event& e);
    };

}