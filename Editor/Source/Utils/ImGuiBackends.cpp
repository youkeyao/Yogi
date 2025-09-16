#include "Utils/ImGuiBackends.h"

#ifdef YG_RENDERER_VULKAN
#    include <backends/imgui_impl_vulkan.cpp>
#endif

#ifdef YG_WINDOW_GLFW
#    include <backends/imgui_impl_glfw.cpp>
#endif