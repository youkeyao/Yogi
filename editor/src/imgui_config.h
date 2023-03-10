#pragma once

#if YG_RENDERER_API == YG_RENDERER_OPENGL
    #include <backends/imgui_impl_opengl3.h>
    #include <backends/imgui_impl_opengl3.cpp>
    #define ImGui_Renderer_Init(...) ImGui_ImplOpenGL3_Init(__VA_ARGS__)
    #define ImGui_Renderer_Shutdown(...) ImGui_ImplOpenGL3_Shutdown(__VA_ARGS__)
    #define ImGui_Renderer_NewFrame(...) ImGui_ImplOpenGL3_NewFrame(__VA_ARGS__)
    #define ImGui_Renderer_Draw(...) ImGui_ImplOpenGL3_RenderDrawData(__VA_ARGS__)

    #if YG_WINDOW_API == YG_WINDOW_GLFW
        #include <backends/imgui_impl_glfw.h>
        #include <backends/imgui_impl_glfw.cpp>
        #define ImGui_Window_Init(window, install_callback) ImGui_ImplGlfw_InitForOpenGL(static_cast<GLFWwindow*>(window), install_callback)
        #define ImGui_Window_Shutdown(...) ImGui_ImplGlfw_Shutdown(__VA_ARGS__)
        #define ImGui_Window_NewFrame(...) ImGui_ImplGlfw_NewFrame(__VA_ARGS__)
        #define ImGui_Window_Render() \
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) { \
                GLFWwindow* backup_current_context = glfwGetCurrentContext(); \
                ImGui::UpdatePlatformWindows(); \
                ImGui::RenderPlatformWindowsDefault(); \
                glfwMakeContextCurrent(backup_current_context); \
            }
    #endif
#endif