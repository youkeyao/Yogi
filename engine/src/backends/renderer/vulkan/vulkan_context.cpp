#include "backends/renderer/vulkan/vulkan_context.h"
#if YG_WINDOW_API == 1
    #include <GLFW/glfw3.h>
#endif

namespace Yogi {

    #define YG_STR(x) YG_XSTR(x)
    #define YG_XSTR(x) #x

    const std::vector<const char*> validation_layers = {
        "VK_LAYER_KHRONOS_validation"
    };
    const std::vector<const char*> device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
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

    bool checkValidationLayerSupport() {
        setenv("VK_LAYER_PATH", YG_STR(YG_VK_LAYER_PATH), 1);

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
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            YG_CORE_ERROR("validation layer: {0}", pCallbackData->pMessage);
        }
        else {
            YG_CORE_INFO("validation layer: {0}", pCallbackData->pMessage);
        }

        return VK_FALSE;
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
        // VkPhysicalDeviceFeatures supportedFeatures;
        // vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
        // return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
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
            #if YG_WINDOW_API == 1
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
        init();
    }

    VulkanContext::~VulkanContext()
    {
        for (auto image_view : m_swap_chain_image_views) {
            vkDestroyImageView(m_device, image_view, nullptr);
        }
        vkDestroySwapchainKHR(m_device, m_swap_chain, nullptr);
        vkDestroyDevice(m_device, nullptr);
        #ifdef YG_DEBUG
            DestroyDebugUtilsMessengerEXT(m_instance, debug_messenger, nullptr);
        #endif
        vkDestroySurfaceKHR(m_instance, surface, nullptr);
        vkDestroyInstance(m_instance, nullptr);
    }

    void VulkanContext::init()
    {
        YG_PROFILE_FUNCTION();

        create_instance();
        setup_debug_messenger();
        create_surface();
        pick_physical_device();
        create_logical_device();
        create_swap_chain();
        create_image_views();
        create_graphics_pipeline();
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
        #if YG_WINDOW_API == 1
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
        #if YG_WINDOW_API == 1
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
        // device_features.samplerAnisotropy = VK_TRUE;

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
        VkExtent2D extent = chooseSwapExtent(swap_chain_support.capabilities, m_window);

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
        create_info.imageExtent = extent;
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
        m_swap_chain_extent = extent;
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

    void VulkanContext::create_graphics_pipeline()
    {

    }

}