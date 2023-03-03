#pragma once

#include <engine.h>

namespace Yogi {

    class ImguiLayer : public Layer
    {
    public:
        ImguiLayer();
        ~ImguiLayer() = default;

        void on_attach() override;
        void on_detach() override;
        void on_update(Timestep ts) override;
        void on_event(Event& event) override;
    private:
        void imgui_begin();
        void imgui_end();
        void set_theme_color();
    };

}