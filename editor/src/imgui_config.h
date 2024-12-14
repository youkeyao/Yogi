#pragma once

#if YG_RENDERER_OPENGL
    #include <backends/imgui_impl_opengl3.h>
    #include <backends/imgui_impl_opengl3.cpp>
    #include "backends/renderer/opengl/opengl_texture.h"
    #define ImGui_Renderer_Init() ImGui_ImplOpenGL3_Init("#version 330")
    #define ImGui_Renderer_Shutdown() ImGui_ImplOpenGL3_Shutdown()
    #define ImGui_Renderer_NewFrame() ImGui_ImplOpenGL3_NewFrame()
    #define ImGui_Renderer_Draw() ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData())
    #define ImGui_Renderer_Texture(texture, viewport_x, viewport_y, tex_x, tex_y) ImGui::Image( \
            (void*)(uint64_t)((OpenGLRenderTexture*)texture.get())->get_renderer_id(), \
            ImVec2( viewport_x, viewport_y ), \
            ImVec2( 0, tex_y ), \
            ImVec2( tex_x, 0 ) \
        )

    #if YG_WINDOW_GLFW
        #include <backends/imgui_impl_glfw.h>
        #include <backends/imgui_impl_glfw.cpp>
        #define ImGui_Window_Init(window) ImGui_ImplGlfw_InitForOpenGL(static_cast<GLFWwindow*>(window), true)
        #define ImGui_Window_Shutdown() ImGui_ImplGlfw_Shutdown()
        #define ImGui_Window_NewFrame() ImGui_ImplGlfw_NewFrame()
        #define ImGui_Window_OnEvent(e)
        #define ImGui_Window_Render() \
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) { \
                GLFWwindow* backup_current_context = glfwGetCurrentContext(); \
                ImGui::UpdatePlatformWindows(); \
                ImGui::RenderPlatformWindowsDefault(); \
                glfwMakeContextCurrent(backup_current_context); \
            }
    #elif YG_WINDOW_SDL
        #include <backends/imgui_impl_sdl2.h>
        #include <backends/imgui_impl_sdl2.cpp>
        #define ImGui_Window_Init(window) ImGui_ImplSDL2_InitForOpenGL(static_cast<SDL_Window*>(window), SDL_GL_GetCurrentContext())
        #define ImGui_Window_Shutdown() ImGui_ImplSDL2_Shutdown()
        #define ImGui_Window_NewFrame() ImGui_ImplSDL2_NewFrame()
        #define ImGui_Window_OnEvent(e) ImGui_ImplSDL2_ProcessEvent((SDL_Event*)e.native_event)
        #define ImGui_Window_Render() \
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) { \
                SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow(); \
                SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext(); \
                ImGui::UpdatePlatformWindows(); \
                ImGui::RenderPlatformWindowsDefault(); \
                SDL_GL_MakeCurrent(backup_current_window, backup_current_context); \
            }
    #endif
#elif YG_RENDERER_VULKAN
    #include "runtime/core/application.h"
    #include "backends/renderer/vulkan/vulkan_context.h"
    #include <backends/imgui_impl_vulkan.h>
    #include <backends/imgui_impl_vulkan.cpp>
    VkDescriptorPool g_DescriptorPool;
    std::map<Yogi::VulkanRenderTexture*, ImTextureID> g_TextureMap;
    void imgui_vulkan_init()
    {
        ImGui_ImplVulkan_InitInfo init_info = {};
        Yogi::VulkanContext* context = (Yogi::VulkanContext*)Yogi::Application::get().get_window().get_context();

        VkExtent2D extent = context->get_swap_chain_extent();
        context->set_has_depth_attachment(false);
        context->create_frame_buffers();
        Yogi::VulkanFrameBuffer* frame_buffer = context->get_current_frame_buffer();

        init_info.Instance = context->get_instance();
        init_info.PhysicalDevice = context->get_physical_device();
        init_info.Device = context->get_device();
        init_info.QueueFamily = 0;
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
        init_info.MSAASamples = context->get_msaa_samples();
        init_info.Allocator = nullptr;
        init_info.CheckVkResultFn = nullptr;
        ImGui_ImplVulkan_Init(&init_info, frame_buffer->get_vk_clear_render_pass());

        VkCommandBuffer command_buffer = context->begin_single_time_commands();
        ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
        context->end_single_time_commands(command_buffer);
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }
    void imgui_vulkan_draw()
    {
        Yogi::VulkanContext* context = (Yogi::VulkanContext*)Yogi::Application::get().get_window().get_context();

        context->set_current_pipeline(nullptr);

        VkCommandBuffer command_buffer = context->begin_render_command();
        Yogi::VulkanFrameBuffer* frame_buffer = context->get_current_frame_buffer();

        VkExtent2D extent = context->get_swap_chain_extent();

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = frame_buffer->get_vk_clear_render_pass();
        renderPassInfo.framebuffer = frame_buffer->get_vk_frame_buffer();
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = extent;
        std::vector<VkClearValue> clear_value{{{0, 0, 0, 1}}, {{0, 0, 0, 1}}};
        renderPassInfo.clearValueCount = 2;
        renderPassInfo.pClearValues = clear_value.data();

        vkCmdBeginRenderPass(command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        VkViewport viewport{0, 0, (float)extent.width, (float)extent.height, 0, 1};
        vkCmdSetViewport(command_buffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = extent;
        vkCmdSetScissor(command_buffer, 0, 1, &scissor);

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);

        vkCmdEndRenderPass(command_buffer);

        context->end_render_command();
    }
    void imgui_vulkan_shutdown()
    {
        Yogi::VulkanContext* context = (Yogi::VulkanContext*)Yogi::Application::get().get_window().get_context();
        vkDeviceWaitIdle(context->get_device());
        ImGui_ImplVulkan_Shutdown();
        vkDestroyDescriptorPool(context->get_device(), g_DescriptorPool, nullptr);
    }
    ImTextureID imgui_vulkan_texture_id(Yogi::VulkanRenderTexture* texture)
    {
        if (g_TextureMap.find(texture) != g_TextureMap.end()) {
            ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)g_TextureMap[texture]);
        }
        g_TextureMap[texture] = ImGui_ImplVulkan_AddTexture(texture->get_vk_sampler(), texture->get_vk_image_view(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        return g_TextureMap[texture];
    }

    #define ImGui_Renderer_Init() imgui_vulkan_init()
    #define ImGui_Renderer_Shutdown() imgui_vulkan_shutdown()
    #define ImGui_Renderer_NewFrame() ImGui_ImplVulkan_NewFrame()
    #define ImGui_Renderer_Draw() imgui_vulkan_draw()
    #define ImGui_Renderer_Texture(texture, viewport_x, viewport_y, tex_x, tex_y) ImGui::Image( \
            imgui_vulkan_texture_id((VulkanRenderTexture*)texture.get()), \
            ImVec2( viewport_x, viewport_y ), \
            ImVec2( 0, tex_y ), \
            ImVec2( tex_x, 0 ) \
        )

    #if YG_WINDOW_GLFW
        #include <backends/imgui_impl_glfw.h>
        #include <backends/imgui_impl_glfw.cpp>
        #define ImGui_Window_Init(window) ImGui_ImplGlfw_InitForVulkan(static_cast<GLFWwindow*>(window), true)
        #define ImGui_Window_Shutdown() ImGui_ImplGlfw_Shutdown()
        #define ImGui_Window_NewFrame() ImGui_ImplGlfw_NewFrame()
        #define ImGui_Window_OnEvent(e)
        #define ImGui_Window_Render() \
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) { \
                ImGui::UpdatePlatformWindows(); \
                ImGui::RenderPlatformWindowsDefault(); \
            }
    #elif YG_WINDOW_SDL
        #include <backends/imgui_impl_sdl2.h>
        #include <backends/imgui_impl_sdl2.cpp>
        #define ImGui_Window_Init(window) ImGui_ImplSDL2_InitForVulkan(static_cast<SDL_Window*>(window))
        #define ImGui_Window_Shutdown() ImGui_ImplSDL2_Shutdown()
        #define ImGui_Window_NewFrame() ImGui_ImplSDL2_NewFrame()
        #define ImGui_Window_OnEvent(e) ImGui_ImplSDL2_ProcessEvent((SDL_Event*)e.native_event)
        #define ImGui_Window_Render() \
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) { \
                ImGui::UpdatePlatformWindows(); \
                ImGui::RenderPlatformWindowsDefault(); \
            }
    #endif
#endif