#pragma once

#include "core/layer.h"

namespace hazel {

    class ImGuiLayer : public Layer
    {
    public:
        ImGuiLayer();
        ~ImGuiLayer();

        void on_attach() override;
        void on_detach() override;
        void on_imgui_render() override;
        
        void begin();
        void end();
    };

}