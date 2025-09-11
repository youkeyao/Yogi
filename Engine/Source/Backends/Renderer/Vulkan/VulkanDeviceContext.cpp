#include "VulkanDeviceContext.h"
#include "VulkanSwapChain.h"

namespace Yogi
{

Handle<IDeviceContext> IDeviceContext::Create() { return Handle<VulkanDeviceContext>::Create(); }

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                    void*                                       pUserData)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        YG_CORE_ERROR("Vulkan: validation layer: {0}", pCallbackData->pMessage);
    }
    else
    {
        YG_CORE_INFO("Vulkan: validation layer: {0}", pCallbackData->pMessage);
    }

    return VK_FALSE;
}

VulkanDeviceContext::VulkanDeviceContext()
{
    volkInitialize();
    CreateVkInstance();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateCommandPool();
    CreateDescriptorPool();
}

VulkanDeviceContext::~VulkanDeviceContext()
{
    vkDeviceWaitIdle(m_device);

    for (auto& pool : m_descriptorPools)
    {
        vkDestroyDescriptorPool(m_device, pool, nullptr);
    }
    m_descriptorPools.clear();

    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    vkDestroyDevice(m_device, nullptr);

#ifdef YG_DEBUG
    vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
#endif

    vkDestroyInstance(m_instance, nullptr);
}

void VulkanDeviceContext::WaitIdle() { vkDeviceWaitIdle(m_device); }

VkDescriptorSet VulkanDeviceContext::AllocateVkDescriptorSet(VkDescriptorSetLayout layout)
{
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = m_descriptorPools.back();
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts        = &layout;

    VkDescriptorSet set;
    if (vkAllocateDescriptorSets(m_device, &allocInfo, &set) != VK_SUCCESS)
    {
        CreateDescriptorPool();
        allocInfo.descriptorPool = m_descriptorPools.back();
        VkResult result          = vkAllocateDescriptorSets(m_device, &allocInfo, &set);
        YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to allocate descriptor set!");
    }
    return set;
}

// -------------------------------------------------------------------------------------

void VulkanDeviceContext::CreateVkInstance()
{
    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "Yogi";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    std::vector<const char*> extensions;
    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef YG_PLATFORM_WIN32
    extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif YG_PLATFORM_MACOS
    extensions.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
#elif YG_PLATFORM_LINUX
    extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
    extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
    extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif

#ifdef YG_DEBUG
#    ifdef YG_PLATFORM_WIN32
    _putenv_s("VK_LAYER_PATH", YG_VK_LAYER_PATH);
#    else
    setenv("VK_LAYER_PATH", YG_VK_LAYER_PATH, 1);
#    endif
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    YG_CORE_ASSERT(CheckValidationLayerSupport(ValidationLayers),
                   "Vulkan: validation layers requested, but not available!");
    debugCreateInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = DebugCallback;

    createInfo.enabledLayerCount   = static_cast<uint32_t>(ValidationLayers.size());
    createInfo.ppEnabledLayerNames = ValidationLayers.data();
    createInfo.pNext               = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#else
    createInfo.enabledLayerCount = 0;
    createInfo.pNext             = nullptr;
#endif

    createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
    YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to create instance!");

    volkLoadInstance(m_instance);

#ifdef YG_DEBUG
    if (vkCreateDebugUtilsMessengerEXT(m_instance, &debugCreateInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
    {
        YG_CORE_ERROR("Vulkan: Failed to set up debug messenger!");
    }
#endif
}

void VulkanDeviceContext::PickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    YG_CORE_ASSERT(deviceCount > 0, "Vulkan: No physical devices found!");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    for (const VkPhysicalDevice& device : devices)
    {
        if (IsDeviceSuitable(device, DeviceExtensions))
        {
            m_physicalDevice = device;
            break;
        }
    }

    YG_CORE_ASSERT(m_physicalDevice != VK_NULL_HANDLE, "Vulkan: Failed to find a suitable GPU!");
}

void VulkanDeviceContext::CreateLogicalDevice()
{
    QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount       = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.independentBlend = VK_TRUE;
    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedFeature{};
    extendedFeature.sType                = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
    extendedFeature.extendedDynamicState = true;
    VkDeviceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos       = queueCreateInfos.data();
    createInfo.pEnabledFeatures        = &deviceFeatures;
    createInfo.pNext                   = &extendedFeature;
    createInfo.enabledExtensionCount   = static_cast<uint32_t>(DeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = DeviceExtensions.data();

#ifdef YG_DEBUG
    createInfo.enabledLayerCount   = static_cast<uint32_t>(ValidationLayers.size());
    createInfo.ppEnabledLayerNames = ValidationLayers.data();
#else
    createInfo.enabledLayerCount = 0;
#endif

    VkResult result = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);
    YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to create logical device!");

    volkLoadDevice(m_device);

    vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_transferQueue);
}

void VulkanDeviceContext::CreateCommandPool()
{
    QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = indices.graphicsFamily.value();
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkResult result = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);
    YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to create command pool!");
}

void VulkanDeviceContext::CreateDescriptorPool()
{
    VkDescriptorPool           descriptorPool;
    VkDescriptorPoolCreateInfo poolInfo = {};

    VkDescriptorPoolSize poolSizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                                         { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                                         { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                                         { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                                         { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                                         { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                                         { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                                         { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                                         { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                                         { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                                         { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets       = 1000 * sizeof(poolSizes) / sizeof(VkDescriptorPoolSize);
    poolInfo.poolSizeCount = sizeof(poolSizes) / sizeof(VkDescriptorPoolSize);
    poolInfo.pPoolSizes    = poolSizes;

    VkResult result = vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &descriptorPool);
    YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to create descriptor pool!");

    m_descriptorPools.push_back(descriptorPool);
}

} // namespace Yogi
