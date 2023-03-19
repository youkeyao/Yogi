#pragma once

#if YG_RENDERER_API == YG_RENDERER_OPENGL
    #include <backends/imgui_impl_opengl3.h>
    #include <backends/imgui_impl_opengl3.cpp>
    #define ImGui_Renderer_Init(...) ImGui_ImplOpenGL3_Init("#version 330")
    #define ImGui_Renderer_Shutdown(...) ImGui_ImplOpenGL3_Shutdown(__VA_ARGS__)
    #define ImGui_Renderer_NewFrame(...) ImGui_ImplOpenGL3_NewFrame(__VA_ARGS__)
    #define ImGui_Renderer_Draw(...) ImGui_ImplOpenGL3_RenderDrawData(__VA_ARGS__)
    #define ImGui_Renderer_Texture(x) x->get_renderer_id()
    #define ImGui_Renderer_Resize()

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
#elif YG_RENDERER_API == YG_RENDERER_VULKAN
    #include "runtime/core/application.h"
    #include "backends/renderer/vulkan/vulkan_context.h"
    #include <backends/imgui_impl_vulkan.h>
    #include <backends/imgui_impl_vulkan.cpp>
    VkDescriptorPool g_DescriptorPool;
    VkRenderPass g_RenderPass;
    VkCommandBuffer g_CommandBuffer;
    std::vector<VkFramebuffer> g_Framebuffers;
    void create_frame_buffers()
    {
        Yogi::VulkanContext* context = (Yogi::VulkanContext*)Yogi::Application::get().get_window().get_context();
        const std::vector<VkImageView>& swap_chain_image_views = context->get_swap_chain_image_views();
        VkFramebufferCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = g_RenderPass;
        info.attachmentCount = 1;
        info.width = context->get_swap_chain_extent().width;
        info.height = context->get_swap_chain_extent().height;
        info.layers = 1;
        g_Framebuffers.resize(swap_chain_image_views.size());
        for (int32_t i = 0; i < swap_chain_image_views.size(); i ++) {
            VkFramebuffer frame_buffer;
            info.pAttachments = &swap_chain_image_views[i];
            vkCreateFramebuffer(context->get_device(), &info, nullptr, &frame_buffer);
            g_Framebuffers[i] = frame_buffer;
        }
    }
    void imgui_vulkan_resize()
    {
        Yogi::VulkanContext* context = (Yogi::VulkanContext*)Yogi::Application::get().get_window().get_context();
        for (auto& frame_buffer : g_Framebuffers) {
            vkDestroyFramebuffer(context->get_device(), frame_buffer, nullptr);
        }
        create_frame_buffers();
    }
    void imgui_vulkan_init()
    {
        ImGui_ImplVulkan_InitInfo init_info = {};
        Yogi::VulkanContext* context = (Yogi::VulkanContext*)Yogi::Application::get().get_window().get_context();

        g_RenderPass = context->create_render_pass({context->get_swap_chain_image_format()}, false);
        g_CommandBuffer = context->add_command_buffer();

        init_info.Instance = context->get_instance();
        init_info.PhysicalDevice = context->get_physical_device();
        init_info.Device = context->get_device();
        init_info.QueueFamily = 1;
        init_info.Queue = context->get_graphics_queue();
        init_info.PipelineCache = VK_NULL_HANDLE;

        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        VkResult err = vkCreateDescriptorPool(context->get_device(), &pool_info, nullptr, &g_DescriptorPool);

        init_info.DescriptorPool = g_DescriptorPool;
        init_info.Subpass = 0;
        init_info.MinImageCount = context->get_swap_chain_image_count();
        init_info.ImageCount = context->get_swap_chain_image_count();
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = nullptr;
        init_info.CheckVkResultFn = nullptr;
        ImGui_ImplVulkan_Init(&init_info, g_RenderPass);

        VkCommandBuffer command_buffer = context->begin_single_time_commands();
        ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
        context->end_single_time_commands(command_buffer);
        ImGui_ImplVulkan_DestroyFontUploadObjects();

        create_frame_buffers();
    }
    void imgui_vulkan_draw()
    {
        Yogi::VulkanContext* context = (Yogi::VulkanContext*)Yogi::Application::get().get_window().get_context();
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        vkResetCommandBuffer(g_CommandBuffer, 0);
        VkResult result = vkBeginCommandBuffer(g_CommandBuffer, &beginInfo);
        YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to begin recording command buffer!");

        VkExtent2D extent = context->get_swap_chain_extent();

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = g_RenderPass;
        renderPassInfo.framebuffer = g_Framebuffers[context->get_current_present_image_index()];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = extent;
        VkClearValue clear_value{{0, 0, 0, 1}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clear_value;

        vkCmdBeginRenderPass(g_CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        VkViewport viewport{0, 0, (float)extent.width, (float)extent.height, 0, 1};
        vkCmdSetViewport(g_CommandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = extent;
        vkCmdSetScissor(g_CommandBuffer, 0, 1, &scissor);

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), g_CommandBuffer);

        vkCmdEndRenderPass(g_CommandBuffer);

        result = vkEndCommandBuffer(g_CommandBuffer);
        YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to record imgui command buffer!");
    }
    void imgui_vulkan_shutdown()
    {
        ImGui_ImplVulkan_Shutdown();
        Yogi::VulkanContext* context = (Yogi::VulkanContext*)Yogi::Application::get().get_window().get_context();
        vkDeviceWaitIdle(context->get_device());
        for (auto& frame_buffer : g_Framebuffers) {
            vkDestroyFramebuffer(context->get_device(), frame_buffer, nullptr);
        }
        vkDestroyRenderPass(context->get_device(), g_RenderPass, nullptr);
        vkFreeCommandBuffers(context->get_device(), context->get_command_pool(), 1, &g_CommandBuffer);
        vkDestroyDescriptorPool(context->get_device(), g_DescriptorPool, nullptr);
    }

    #define ImGui_Renderer_Init(...) imgui_vulkan_init()
    #define ImGui_Renderer_Shutdown(...) imgui_vulkan_shutdown()
    #define ImGui_Renderer_NewFrame(...) ImGui_ImplVulkan_NewFrame(__VA_ARGS__)
    #define ImGui_Renderer_Draw(...) imgui_vulkan_draw()
    #define ImGui_Renderer_Texture(x) ImGui_ImplVulkan_AddTexture(((VulkanTexture2D*)x.get())->get_vk_sampler(), ((VulkanTexture2D*)x.get())->get_vk_image_view(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    #define ImGui_Renderer_Resize() imgui_vulkan_resize()

    #if YG_WINDOW_API == YG_WINDOW_GLFW
        #include <backends/imgui_impl_glfw.h>
        #include <backends/imgui_impl_glfw.cpp>
        #define ImGui_Window_Init(window, install_callback) ImGui_ImplGlfw_InitForVulkan(static_cast<GLFWwindow*>(window), install_callback)
        #define ImGui_Window_Shutdown(...) ImGui_ImplGlfw_Shutdown(__VA_ARGS__)
        #define ImGui_Window_NewFrame(...) ImGui_ImplGlfw_NewFrame(__VA_ARGS__)
        #define ImGui_Window_Render() \
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) { \
                ImGui::UpdatePlatformWindows(); \
                ImGui::RenderPlatformWindowsDefault(); \
            }
    #endif
#endif