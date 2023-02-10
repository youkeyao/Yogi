#pragma once

#include "base/core/layer.h"

namespace Yogi {

    class ImGuiLayer : public Layer
    {
    public:
        ImGuiLayer();
        ~ImGuiLayer();

        void on_attach() override;
        void on_detach() override;
        
        void begin();
        void end();
    };

}