#pragma once

#include "core/application.h"
#include "core/layer.h"
#include <imgui.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_glfw.h>
#include <GLFW/glfw3.h>

namespace hazel {

    class ImGuiLayer : public Layer
    {
    public:
        ImGuiLayer();
        ~ImGuiLayer();

        void on_attach() override;
        void on_detach() override;
        void on_update() override;
        void on_event(Event& e) override;
    private:
        float m_time = 0.0f;
    };

}