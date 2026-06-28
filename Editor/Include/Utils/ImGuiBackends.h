#pragma once

#include <imgui.h>

#ifdef YG_WINDOW_GLFW
#    include <backends/imgui_impl_glfw.h>
#endif

#include <Yogi.h>

inline void ImGuiImage(const Yogi::IShaderResourceBinding& srb,
                       const ImVec2&                       size,
                       const ImVec2&                       uv0 = { 0, 0 },
                       const ImVec2&                       uv1 = { 1, 1 })
{
    ImGui::Image((ImTextureID)(intptr_t)(&srb), size, uv0, uv1);
}
