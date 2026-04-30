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
#    include "Backends/Renderer/Vulkan/VulkanShaderResourceBinding.h"

#    define IMGUI_IMPL_VULKAN_NO_PROTOTYPES
#    include <backends/imgui_impl_vulkan.h>

inline void ImGuiImage(const Yogi::ITexture&               texture,
                       const Yogi::IShaderResourceBinding& shaderResourceBinding,
                       const ImVec2&                       size,
                       const ImVec2&                       uv0 = { 0, 0 },
                       const ImVec2&                       uv1 = { 1, 1 })
{
    Yogi::Owner<Yogi::ICommandBuffer> commandBuffer = Yogi::Owner<Yogi::ICommandBuffer>::Create(
        Yogi::CommandBufferDesc{ Yogi::CommandBufferUsage::OneTimeSubmit, Yogi::SubmitQueue::Graphics });
    commandBuffer->Begin();
    commandBuffer->Blit(&texture, &texture);
    commandBuffer->End();
    commandBuffer->Submit();

    const Yogi::VulkanShaderResourceBinding& vkSRB =
        static_cast<const Yogi::VulkanShaderResourceBinding&>(shaderResourceBinding);
    ImGui::Image((void*)(intptr_t)vkSRB.GetVkDescriptorSet(),
                 size,
                 uv0,
                 uv1,
                 ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
                 ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
}
#endif