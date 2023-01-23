#include "base/imgui/imgui_layer.h"
#include "base/core/application.h"
#include <imgui.h>
#if HZ_RENDERER_API == 1
    #include <backends/imgui_impl_opengl3.h>
    #include <backends/imgui_impl_glfw.h>
    #include <GLFW/glfw3.h>
#endif

namespace hazel {

    ImGuiLayer::ImGuiLayer() : Layer("ImGuiLayer") {}
    ImGuiLayer::~ImGuiLayer() {}

    void ImGuiLayer::on_attach()
    {
        HZ_PROFILE_FUNCTION();

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        (void) io;

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui::StyleColorsDark();

        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        Application& app = Application::get();
        #if HZ_RENDERER_API == 1
            GLFWwindow* window = static_cast<GLFWwindow*>(app.get_window().get_native_window());

            ImGui_ImplGlfw_InitForOpenGL(window, true);
            ImGui_ImplOpenGL3_Init("#version 130");
        #endif
    }

    void ImGuiLayer::on_detach()
    {
        HZ_PROFILE_FUNCTION();

        #if HZ_RENDERER_API == 1
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
        #endif

        ImGui::DestroyContext();
    }

    void ImGuiLayer::begin()
    {
        HZ_PROFILE_FUNCTION();

        #if HZ_RENDERER_API == 1
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
        #endif

        ImGui::NewFrame();
    }

    void ImGuiLayer::end()
    {
        HZ_PROFILE_FUNCTION();
        
        ImGuiIO& io = ImGui::GetIO();
        Application& app = Application::get();
        io.DisplaySize = ImVec2(app.get_window().get_width(), app.get_window().get_height());

        ImGui::Render();
        #if HZ_RENDERER_API == 1
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                GLFWwindow* backup_current_context = glfwGetCurrentContext();
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
                glfwMakeContextCurrent(backup_current_context);
            }
        #endif
    }

}