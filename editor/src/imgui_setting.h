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
    static void show_image(const Ref<RenderTexture> &texture, ImVec2 viewport, ImVec2 texcoords);
    static void imgui_on_event(Event &e);
};

}  // namespace Yogi