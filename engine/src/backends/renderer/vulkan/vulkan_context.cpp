#include "backends/renderer/vulkan/vulkan_context.h"
#if YG_WINDOW_API == YG_WINDOW_GLFW
    #include <GLFW/glfw3.h>
#endif

namespace Yogi {

    const int MAX_FRAMES_IN_FLIGHT = 2;

    const std::vector<const char*> validation_layers = {
        "VK_LAYER_KHRONOS_validation"
    };
    const std::vector<const char*> device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    std::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkDebugUtilsMessengerEXT debug_messenger;
    VkSurfaceKHR surface;
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            YG_CORE_ERROR("validation layer: {0}", pCallbackData->pMessage);
        }
        // else {
        //     YG_CORE_INFO("validation layer: {0}", pCallbackData->pMessage);
        // }

        return VK_FALSE;
    }
    bool checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> available_layers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, available_layers.data());

        for (const char* layer_name : validation_layers) {
            bool layer_found = false;
            for (const auto& layer_properties : available_layers) {
                if (strcmp(layer_name, layer_properties.layerName) == 0) {
                    layer_found = true;
                    break;
                }
            }
            if (!layer_found) {
                return false;
            }
        }

        return true;
    }
    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(device_extensions.begin(), device_extensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport) {
                indices.presentFamily = i;
            }
            if (indices.isComplete()) {
                break;
            }
            i++;
        }
        return indices;
    }
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }
        return details;
    }
    bool isDeviceSuitable(VkPhysicalDevice device) {
        QueueFamilyIndices indices = findQueueFamilies(device);
        bool extensionsSupported = checkDeviceExtensionSupport(device);
        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }
        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, Window* window) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            #if YG_WINDOW_API == YG_WINDOW_GLFW
                glfwGetFramebufferSize((GLFWwindow*)window->get_native_window(), &width, &height);
            #endif

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    //----------------------------------------------------------------------------------------

    Scope<GraphicsContext> GraphicsContext::create(Window* window)
    {
        return CreateScope<VulkanContext>(window);
    }

    VulkanContext::VulkanContext(Window* window) : m_window(window)
    {
        setenv("VK_LAYER_PATH", YG_VK_LAYER_PATH, 1);
        init();

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(m_physical_device, &properties);
        YG_CORE_INFO("Vulkan Info:");
        YG_CORE_INFO("    Vendor:   {0}", properties.vendorID);
        YG_CORE_INFO("    Renderer: {0}", properties.deviceName);
        YG_CORE_INFO("    Version:  {0}", properties.apiVersion);

        begin_command_buffer();
    }

    VulkanContext::~VulkanContext()
    {
        end_command_buffer();
        
        vkDeviceWaitIdle(m_device);

        cleanup_swap_chain();
        vkDestroyRenderPass(m_device, m_render_pass, nullptr);
        for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i ++) {
            vkDestroySemaphore(m_device, m_image_available_semaphores[i], nullptr);
            vkDestroySemaphore(m_device, m_render_finished_semaphores[i], nullptr);
            vkDestroyFence(m_device, m_in_flight_fences[i], nullptr);
        }
        vkDestroyCommandPool(m_device, m_command_pool, nullptr);

        vkDestroyDevice(m_device, nullptr);
        #ifdef YG_DEBUG
            DestroyDebugUtilsMessengerEXT(m_instance, debug_messenger, nullptr);
        #endif
        vkDestroySurfaceKHR(m_instance, surface, nullptr);
        vkDestroyInstance(m_instance, nullptr);
    }

    uint32_t VulkanContext::find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties mem_properties;
        vkGetPhysicalDeviceMemoryProperties(m_physical_device, &mem_properties);

        for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
            if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        YG_CORE_ERROR("Failed to find suitable memory type!");
        return 0;
    }

    void VulkanContext::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory)
    {
        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = size;
        buffer_info.usage = usage;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkResult result = vkCreateBuffer(m_device, &buffer_info, nullptr, &buffer);
        YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to create buffer!");

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(m_device, buffer, &mem_requirements);

        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, properties);

        result = vkAllocateMemory(m_device, &alloc_info, nullptr, &buffer_memory);
        YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to allocate buffer memory!");

        vkBindBufferMemory(m_device, buffer, buffer_memory, 0);
    }

    void VulkanContext::copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size)
    {
        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandPool = m_command_pool;
        alloc_info.commandBufferCount = 1;

        VkCommandBuffer command_buffer;
        vkAllocateCommandBuffers(m_device, &alloc_info, &command_buffer);

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(command_buffer, &begin_info);
        VkBufferCopy copy_region{};
        copy_region.size = size;
        vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);
        vkEndCommandBuffer(command_buffer);

        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;

        vkQueueSubmit(m_graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_graphics_queue);

        vkFreeCommandBuffers(m_device, m_command_pool, 1, &command_buffer);
    }

    void VulkanContext::init()
    {
        create_instance();
        setup_debug_messenger();
        create_surface();
        pick_physical_device();
        create_logical_device();
        create_swap_chain();
        create_image_views();
        create_render_pass();
        create_frame_buffers();
        create_command_pool();
        create_command_buffer();
        create_sync_objects();

        m_current_frame_buffer = m_swap_chain_frame_buffers[0];
        m_viewport.width = m_window->get_width();
        m_viewport.height = m_window->get_height();
    }

    void VulkanContext::swap_buffers()
    {
        end_command_buffer();

        VkResult result = vkAcquireNextImageKHR(m_device, m_swap_chain, UINT64_MAX, m_image_available_semaphores[m_current_frame], VK_NULL_HANDLE, &m_image_index);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreate_swap_chain();
            return;
        } else {
            YG_CORE_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Failed to present swap chain image!");
        }

        vkResetFences(m_device, 1, &m_in_flight_fences[m_current_frame]);

        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pCommandBuffers = &m_command_buffers[m_current_frame];

        VkSemaphore wait_semaphores[] = {m_image_available_semaphores[m_current_frame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = wait_semaphores;
        submit_info.pWaitDstStageMask = waitStages;
        submit_info.commandBufferCount = 1;

        VkSemaphore signal_semaphores[] = {m_render_finished_semaphores[m_current_frame]};
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = signal_semaphores;

        result = vkQueueSubmit(m_graphics_queue, 1, &submit_info, m_in_flight_fences[m_current_frame]);
        YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to submit draw command buffer!");

        VkPresentInfoKHR present_info{};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = signal_semaphores;

        VkSwapchainKHR swap_chains[] = {m_swap_chain};
        present_info.swapchainCount = 1;
        present_info.pSwapchains = swap_chains;
        present_info.pImageIndices = &m_image_index;
        present_info.pResults = nullptr;

        result = vkQueuePresentKHR(m_present_queue, &present_info);
        
        if (result == VK_ERROR_OUT_OF_DATE_KHR || is_window_resized) {
            is_window_resized = false;
            recreate_swap_chain();
        } else {
            YG_CORE_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Failed to present swap chain image!");
        }

        m_current_frame = (m_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;

        vkWaitForFences(m_device, 1, &m_in_flight_fences[m_current_frame], VK_TRUE, UINT64_MAX);
        vkResetCommandBuffer(m_command_buffers[m_current_frame], 0);
        
        begin_command_buffer();
    }

    void VulkanContext::create_instance()
    {
        VkApplicationInfo app_info{};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "Yogi";
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName = "Engine";
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;

        uint32_t extension_count = 0;
        const char** extensions;
        #if YG_WINDOW_API == YG_WINDOW_GLFW
            extensions = glfwGetRequiredInstanceExtensions(&extension_count);
        #endif
        std::vector<const char*> extension_lists(extensions, extensions + extension_count);

        #ifdef YG_DEBUG
            YG_CORE_ASSERT(checkValidationLayerSupport(), "Validation layer is not available!");
            create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
            create_info.ppEnabledLayerNames = validation_layers.data();
            extension_lists.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        #else
            create_info.enabledLayerCount = 0;
            create_info.pNext = nullptr;
        #endif

        create_info.enabledExtensionCount = static_cast<uint32_t>(extension_lists.size());
        create_info.ppEnabledExtensionNames = extension_lists.data();

        VkResult result = vkCreateInstance(&create_info, nullptr, &m_instance);
        YG_CORE_ASSERT(result == VK_SUCCESS, "Could not create vulkan instance!");
    }

    void VulkanContext::setup_debug_messenger() {
        #ifdef YG_DEBUG
            VkDebugUtilsMessengerCreateInfoEXT create_info;
            populateDebugMessengerCreateInfo(create_info);

            VkResult result = CreateDebugUtilsMessengerEXT(m_instance, &create_info, nullptr, &debug_messenger);
            YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to set up debug messenger!!");
        #endif
    }

    void VulkanContext::create_surface()
    {
        #if YG_WINDOW_API == YG_WINDOW_GLFW
            VkResult result = glfwCreateWindowSurface(m_instance, (GLFWwindow*)m_window->get_native_window(), nullptr, &surface);
        #endif
        YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to create window surface!!");
    }

    void VulkanContext::pick_physical_device() {
        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);
        YG_CORE_ASSERT(device_count != 0, "Failed to find GPUs with Vulkan support!");
        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(m_instance, &device_count, devices.data());

        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                m_physical_device = device;
                break;
            }
        }

        YG_CORE_ASSERT(m_physical_device != VK_NULL_HANDLE, "Failed to to find a suitable GPU!");
    }

    void VulkanContext::create_logical_device()
    {
        QueueFamilyIndices indices = findQueueFamilies(m_physical_device);

        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        std::set<uint32_t> unique_queue_families = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : unique_queue_families) {
            VkDeviceQueueCreateInfo queue_create_info{};
            queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueFamilyIndex = queueFamily;
            queue_create_info.queueCount = 1;
            queue_create_info.pQueuePriorities = &queuePriority;
            queue_create_infos.push_back(queue_create_info);
        }

        VkPhysicalDeviceFeatures device_features{};

        VkDeviceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
        create_info.pQueueCreateInfos = queue_create_infos.data();
        create_info.pEnabledFeatures = &device_features;
        create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
        create_info.ppEnabledExtensionNames = device_extensions.data();
        #ifdef YG_DEBUG
            create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
            create_info.ppEnabledLayerNames = validation_layers.data();
        #else
            create_info.enabledLayerCount = 0;
        #endif

        VkResult result = vkCreateDevice(m_physical_device, &create_info, nullptr, &m_device);
        YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to create logical device!");

        vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphics_queue);
        vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_present_queue);
    }

    void VulkanContext::create_swap_chain()
    {
        SwapChainSupportDetails swap_chain_support = querySwapChainSupport(m_physical_device);

        VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(swap_chain_support.formats);
        VkPresentModeKHR present_mode = chooseSwapPresentMode(swap_chain_support.presentModes);
        m_swap_chain_extent = chooseSwapExtent(swap_chain_support.capabilities, m_window);

        uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
        if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount) {
            image_count = swap_chain_support.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.surface = surface;

        create_info.minImageCount = image_count;
        create_info.imageFormat = surface_format.format;
        create_info.imageColorSpace = surface_format.colorSpace;
        create_info.imageExtent = m_swap_chain_extent;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = findQueueFamilies(m_physical_device);
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        if (indices.graphicsFamily != indices.presentFamily) {
            create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = 2;
            create_info.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        create_info.preTransform = swap_chain_support.capabilities.currentTransform;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info.presentMode = present_mode;
        create_info.clipped = VK_TRUE;

        VkResult result = vkCreateSwapchainKHR(m_device, &create_info, nullptr, &m_swap_chain);
        YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to create swap chain!");

        vkGetSwapchainImagesKHR(m_device, m_swap_chain, &image_count, nullptr);
        m_swap_chain_images.resize(image_count);
        vkGetSwapchainImagesKHR(m_device, m_swap_chain, &image_count, m_swap_chain_images.data());

        m_swap_chain_image_format = surface_format.format;
    }

    void VulkanContext::create_image_views()
    {
        m_swap_chain_image_views.resize(m_swap_chain_images.size());
        for (size_t i = 0; i < m_swap_chain_images.size(); i++) {
            VkImageViewCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            create_info.image = m_swap_chain_images[i];
            create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            create_info.format = m_swap_chain_image_format;
            create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            create_info.subresourceRange.baseMipLevel = 0;
            create_info.subresourceRange.levelCount = 1;
            create_info.subresourceRange.baseArrayLayer = 0;
            create_info.subresourceRange.layerCount = 1;

            VkResult result = vkCreateImageView(m_device, &create_info, nullptr, &m_swap_chain_image_views[i]);
            YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to create image views!");
        }
    }

    void VulkanContext::create_render_pass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = m_swap_chain_image_format;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        VkResult result = vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_render_pass);
        YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to create render pass!");
    }

    void VulkanContext::create_frame_buffers() {
        m_swap_chain_frame_buffers.resize(m_swap_chain_image_views.size());
        for (size_t i = 0; i < m_swap_chain_image_views.size(); i++) {
            VkImageView attachments[] = {
                m_swap_chain_image_views[i]
            };

            VkFramebufferCreateInfo frame_buffer_info{};
            frame_buffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            frame_buffer_info.renderPass = m_render_pass;
            frame_buffer_info.attachmentCount = 1;
            frame_buffer_info.pAttachments = attachments;
            frame_buffer_info.width = m_swap_chain_extent.width;
            frame_buffer_info.height = m_swap_chain_extent.height;
            frame_buffer_info.layers = 1;

            VkResult result = vkCreateFramebuffer(m_device, &frame_buffer_info, nullptr, &m_swap_chain_frame_buffers[i]);
            YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to create graphics pipeline!");
        }
    }

    void VulkanContext::create_command_pool()
    {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physical_device);

        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        VkResult result = vkCreateCommandPool(m_device, &pool_info, nullptr, &m_command_pool);
        YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to create command pool!");
    }

    void VulkanContext::create_command_buffer()
    {
        m_command_buffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = m_command_pool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = (uint32_t) m_command_buffers.size();

        VkResult result = vkAllocateCommandBuffers(m_device, &alloc_info, m_command_buffers.data());
        YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to allocate command buffers!");
    }

    void VulkanContext::create_sync_objects()
    {
        m_image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i ++) {
            YG_CORE_ASSERT((vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_image_available_semaphores[i]) == VK_SUCCESS &&
                vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_render_finished_semaphores[i]) == VK_SUCCESS &&
                vkCreateFence(m_device, &fenceInfo, nullptr, &m_in_flight_fences[i]) == VK_SUCCESS)
            , "Failed to create semaphores!");
        }
    }

    void VulkanContext::cleanup_swap_chain()
    {
        for (size_t i = 0; i < m_swap_chain_frame_buffers.size(); i++) {
            vkDestroyFramebuffer(m_device, m_swap_chain_frame_buffers[i], nullptr);
        }

        for (size_t i = 0; i < m_swap_chain_image_views.size(); i++) {
            vkDestroyImageView(m_device, m_swap_chain_image_views[i], nullptr);
        }

        vkDestroySwapchainKHR(m_device, m_swap_chain, nullptr);
    }

    void VulkanContext::recreate_swap_chain()
    {
        vkDeviceWaitIdle(m_device);

        bool is_swap_chain_frame_buffer = m_current_frame_buffer == m_swap_chain_frame_buffers[0];

        cleanup_swap_chain();

        create_swap_chain();
        create_image_views();
        create_frame_buffers();

        if (is_swap_chain_frame_buffer) m_current_frame_buffer = m_swap_chain_frame_buffers[0];
    }

    void VulkanContext::begin_command_buffer()
    {
        VkCommandBuffer command_buffer = m_command_buffers[m_current_frame];

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = 0;
        begin_info.pInheritanceInfo = nullptr;

        VkResult result = vkBeginCommandBuffer(command_buffer, &begin_info);
        YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to begin recording command buffer!");

        VkRenderPassBeginInfo render_begin_info{};
        render_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_begin_info.renderPass = m_render_pass;
        render_begin_info.framebuffer = m_current_frame_buffer;
        render_begin_info.renderArea.offset = { 0, 0 };
        render_begin_info.renderArea.extent = m_swap_chain_extent;
        render_begin_info.clearValueCount = 1;
        render_begin_info.pClearValues = &m_clear_color;
        vkCmdBeginRenderPass(command_buffer, &render_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdSetViewport(command_buffer, 0, 1, &m_viewport);
        VkRect2D scissor{};
        scissor.offset = { (int32_t)m_viewport.x, (int32_t)m_viewport.y };
        scissor.extent = { (uint32_t)m_viewport.width, (uint32_t)m_viewport.height };
        vkCmdSetScissor(command_buffer, 0, 1, &scissor);
    }

    void VulkanContext::end_command_buffer()
    {
        VkCommandBuffer command_buffer = m_command_buffers[m_current_frame];

        vkCmdEndRenderPass(m_command_buffers[m_current_frame]);
        
        VkResult result = vkEndCommandBuffer(command_buffer);
        YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to record command buffer!");
    }

}