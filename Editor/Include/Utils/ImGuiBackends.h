#pragma once

#include <imgui.h>

#ifdef YG_WINDOW_GLFW
#    include <backends/imgui_impl_glfw.h>
#endif

#ifdef YG_RENDERER_VULKAN
#    include "Backends/Renderer/Vulkan/VulkanDeviceContext.h"
#    include "Backends/Renderer/Vulkan/VulkanSwapChain.h"
#    include "Backends/Renderer/Vulkan/VulkanCommandBuffer.h"
#    include "Backends/Renderer/Vulkan/VulkanRenderPass.h"
#    include "Backends/Renderer/Vulkan/VulkanFrameBuffer.h"

#    define IMGUI_IMPL_VULKAN_NO_PROTOTYPES
#    include <vulkan/vulkan.h>
#    include <backends/imgui_impl_vulkan.h>
#endif