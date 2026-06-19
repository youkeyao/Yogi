#pragma once

#include <imgui.h>

#ifdef YG_WINDOW_GLFW
#    include <backends/imgui_impl_glfw.h>
#endif

#ifdef YG_RENDERER_VULKAN
#    include "Backends/Renderer/Vulkan/VulkanDeviceContext.h"
#    include "Backends/Renderer/Vulkan/VulkanSwapChain.h"
#    include "Backends/Renderer/Vulkan/VulkanCommandBuffer.h"
#    include "Backends/Renderer/Vulkan/VulkanTextureView.h"

#    define IMGUI_IMPL_VULKAN_NO_PROTOTYPES
#    include <backends/imgui_impl_vulkan.h>

inline void ImGuiImage(const Yogi::ITextureView& view,
                       const ImVec2&             size,
                       const ImVec2&             uv0 = { 0, 0 },
                       const ImVec2&             uv1 = { 1, 1 })
{
    {
        Yogi::Owner<Yogi::ICommandBuffer> commandBuffer = Yogi::Owner<Yogi::ICommandBuffer>::Create(
            Yogi::CommandBufferDesc{ Yogi::CommandBufferUsage::OneTimeSubmit, Yogi::SubmitQueue::Graphics });
        commandBuffer->Begin();
        commandBuffer->Barrier(Yogi::BarrierDesc{
            .TextureView = &view,
            .BeforeState = Yogi::ResourceState::ColorAttachment,
            .AfterState  = Yogi::ResourceState::FragmentShaderResource,
        });
        commandBuffer->End();
        commandBuffer->Submit();
    }

    const Yogi::VulkanTextureView& vkView = static_cast<const Yogi::VulkanTextureView&>(view);
    auto* ctx = static_cast<Yogi::VulkanDeviceContext*>(Yogi::Application::GetInstance().GetContext());

    static std::unordered_map<const Yogi::ITextureView*, VkDescriptorSet> s_cache;

    VkDescriptorSet descriptor = VK_NULL_HANDLE;
    if (auto it = s_cache.find(&view); it != s_cache.end())
    {
        descriptor = it->second;
    }
    else
    {
        descriptor     = ImGui_ImplVulkan_AddTexture(ctx->GetSampler(Yogi::SamplerReductionMode::None),
                                                     vkView.GetVkImageView(),
                                                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        s_cache[&view] = descriptor;
    }

    ImGui::Image(
        (void*)(intptr_t)descriptor, size, uv0, uv1, ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
}

#endif