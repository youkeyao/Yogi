#pragma once

#if YG_RENDERER_API == YG_RENDERER_OPENGL
    #include <backends/imgui_impl_opengl3.h>
    #include <backends/imgui_impl_opengl3.cpp>
    #define ImGui_Renderer_Init(...) ImGui_ImplOpenGL3_Init("#version 330")
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
#elif YG_RENDERER_API == YG_RENDERER_VULKAN
    #include "runtime/core/application.h"
    #include "backends/renderer/vulkan/vulkan_context.h"
    #include <backends/imgui_impl_vulkan.h>
    #include <backends/imgui_impl_vulkan.cpp>
    VkDescriptorPool g_DescriptorPool;
    void imgui_vulkan_init()
    {
        ImGui_ImplVulkan_InitInfo init_info = {};
        Yogi::VulkanContext* context = (Yogi::VulkanContext*)Yogi::Application::get().get_window().get_context();

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
        ImGui_ImplVulkan_Init(&init_info, context->get_render_pass());

        VkCommandBuffer command_buffer = context->begin_single_time_commands();
        ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
        context->end_single_time_commands(command_buffer);
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }
    void frame_render()
    {
        Yogi::VulkanContext* context = (Yogi::VulkanContext*)Yogi::Application::get().get_window().get_context();
        context->add_render_func([](VkCommandBuffer command_buffer){
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);
        });
    }

    #define ImGui_Renderer_Init(...) imgui_vulkan_init()
    #define ImGui_Renderer_Shutdown(...) \
        ImGui_ImplVulkan_Shutdown(__VA_ARGS__); \
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context(); \
        vkDeviceWaitIdle(context->get_device()); \
        vkDestroyDescriptorPool(context->get_device(), g_DescriptorPool, nullptr);
    #define ImGui_Renderer_NewFrame(...) ImGui_ImplVulkan_NewFrame(__VA_ARGS__)
    #define ImGui_Renderer_Draw(...) frame_render()

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