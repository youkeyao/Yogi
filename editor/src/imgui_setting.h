#pragma once

namespace Yogi {

    class ImguiSetting
    {
    public:
        static void init();
        static void shutdown();
        static void imgui_begin();
        static void imgui_end();
    };

}