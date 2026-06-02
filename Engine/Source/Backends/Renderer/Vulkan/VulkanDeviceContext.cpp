#include "VulkanDeviceContext.h"
#include "VulkanSwapChain.h"

#include <volk.h>
#ifdef YG_WINDOW_GLFW
#    include <GLFW/glfw3.h>
#endif

namespace Yogi
{

Owner<IDeviceContext> IDeviceContext::Create(const Window* window)
{
    return Owner<VulkanDeviceContext>::Create(window);
}

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

VulkanDeviceContext::VulkanDeviceContext(const Window* window)
{
    volkInitialize();
    CreateVkInstance();
    CreateVkSurface(window);
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateCommandPools();
    CreateDescriptorPool();
}

VulkanDeviceContext::~VulkanDeviceContext()
{
    vkDeviceWaitIdle(m_device);

    for (VkSampler& s : m_samplers)
    {
        if (s != VK_NULL_HANDLE)
        {
            vkDestroySampler(m_device, s, nullptr);
            s = VK_NULL_HANDLE;
        }
    }

    for (auto& pool : m_descriptorPools)
    {
        vkDestroyDescriptorPool(m_device, pool, nullptr);
    }
    m_descriptorPools.clear();

    if (m_transferCommandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(m_device, m_transferCommandPool, nullptr);
        m_transferCommandPool = VK_NULL_HANDLE;
    }
    if (m_graphicsCommandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(m_device, m_graphicsCommandPool, nullptr);
        m_graphicsCommandPool = VK_NULL_HANDLE;
    }
    vkDestroyDevice(m_device, nullptr);

#ifdef YG_DEBUG
    vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
#endif

    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);
}

void VulkanDeviceContext::WaitIdle()
{
    vkDeviceWaitIdle(m_device);
}

VkDescriptorSet VulkanDeviceContext::AllocateVkDescriptorSet(VkDescriptorSetLayout layout,
                                                             uint32_t              variableDescriptorCount)
{
    VkDescriptorSetVariableDescriptorCountAllocateInfo countInfo{};
    countInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
    countInfo.descriptorSetCount = 1;
    countInfo.pDescriptorCounts  = &variableDescriptorCount;

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = m_descriptorPools.back();
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts        = &layout;
    if (variableDescriptorCount > 0)
        allocInfo.pNext = &countInfo;

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

void VulkanDeviceContext::CreateVkInstance()
{
    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "Yogi";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    std::vector<const char*> extensions;
#ifdef YG_WINDOW_GLFW
    uint32_t     glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    for (uint32_t i = 0; i < glfwExtensionCount; i++)
    {
        extensions.push_back(glfwExtensions[i]);
    }
#endif

#ifdef YG_DEBUG
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

void VulkanDeviceContext::CreateVkSurface(const Window* window)
{
    VkResult result = VK_ERROR_INITIALIZATION_FAILED;
#ifdef YG_WINDOW_GLFW
    result = glfwCreateWindowSurface(m_instance, (GLFWwindow*)window->GetNativeWindow(), nullptr, &m_surface);
#endif

    YG_CORE_ASSERT(result == VK_SUCCESS && m_surface != VK_NULL_HANDLE, "Vulkan: Failed to create window surface!");
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
        if (IsDeviceSuitable(device, m_surface, DeviceExtensions))
        {
            m_physicalDevice = device;
            break;
        }
    }

    YG_CORE_ASSERT(m_physicalDevice != VK_NULL_HANDLE, "Vulkan: Failed to find a suitable GPU!");
}

void VulkanDeviceContext::CreateLogicalDevice()
{
    QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice, m_surface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t>                   uniqueQueueFamilies = { indices.graphicsFamily.value(),
                                                                 indices.computeFamily.value(),
                                                                 indices.transferFamily.value(),
                                                                 indices.presentFamily.value() };

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
    deviceFeatures.independentBlend  = VK_TRUE;
    deviceFeatures.multiDrawIndirect = VK_TRUE;
    deviceFeatures.shaderInt64       = VK_TRUE;

    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    deviceFeatures2.sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.features = deviceFeatures;
    VkPhysicalDeviceVulkan11Features vulkan11Features{};
    vulkan11Features.sType                    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    vulkan11Features.shaderDrawParameters     = VK_TRUE;
    vulkan11Features.storageBuffer16BitAccess = VK_TRUE;
    deviceFeatures2.pNext                     = &vulkan11Features;
    VkPhysicalDeviceVulkan12Features vulkan12Features{};
    vulkan12Features.sType                           = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    vulkan12Features.storageBuffer8BitAccess         = VK_TRUE;
    vulkan12Features.shaderFloat16                   = VK_TRUE;
    vulkan12Features.shaderInt8                      = VK_TRUE;
    vulkan12Features.drawIndirectCount               = VK_TRUE;
    vulkan12Features.samplerFilterMinmax             = VK_TRUE;
    vulkan12Features.bufferDeviceAddress             = VK_TRUE;
    vulkan12Features.descriptorIndexing              = VK_TRUE;
    vulkan12Features.runtimeDescriptorArray          = VK_TRUE;
    vulkan12Features.descriptorBindingPartiallyBound = VK_TRUE;
    vulkan12Features.descriptorBindingSampledImageUpdateAfterBind  = VK_TRUE;
    vulkan12Features.descriptorBindingStorageImageUpdateAfterBind  = VK_TRUE;
    vulkan12Features.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
    vulkan12Features.descriptorBindingVariableDescriptorCount      = VK_TRUE;
    vulkan12Features.shaderSampledImageArrayNonUniformIndexing     = VK_TRUE;
    vulkan11Features.pNext                                         = &vulkan12Features;
    VkPhysicalDeviceVulkan13Features vulkan13Features{};
    vulkan13Features.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    vulkan13Features.dynamicRendering = VK_TRUE;
    vulkan13Features.synchronization2 = VK_TRUE;
    vulkan12Features.pNext            = &vulkan13Features;
    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedFeature{};
    extendedFeature.sType                = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
    extendedFeature.extendedDynamicState = true;
    vulkan13Features.pNext               = &extendedFeature;
    VkPhysicalDeviceMeshShaderFeaturesEXT meshFeature{};
    meshFeature.sType      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
    meshFeature.taskShader = VK_TRUE;
    meshFeature.meshShader = VK_TRUE;
    extendedFeature.pNext  = &meshFeature;
    VkDeviceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos       = queueCreateInfos.data();
    createInfo.pEnabledFeatures        = nullptr;
    createInfo.pNext                   = &deviceFeatures2;
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
    vkGetDeviceQueue(m_device, indices.transferFamily.value(), 0, &m_transferQueue);
}

void VulkanDeviceContext::CreateCommandPools()
{
    QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice, m_surface);

    auto makePool = [this](uint32_t familyIndex) {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = familyIndex;
        poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VkCommandPool pool   = VK_NULL_HANDLE;
        VkResult      result = vkCreateCommandPool(m_device, &poolInfo, nullptr, &pool);
        YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to create command pool!");
        return pool;
    };

    m_graphicsCommandPool = makePool(indices.graphicsFamily.value());

    if (indices.transferFamily.value() != indices.graphicsFamily.value())
    {
        m_transferCommandPool = makePool(indices.transferFamily.value());
    }
}

VkCommandPool VulkanDeviceContext::GetVkCommandPoolForQueue(SubmitQueue queue) const
{
    switch (queue)
    {
        case SubmitQueue::Graphics:
            return m_graphicsCommandPool;
        case SubmitQueue::Transfer:
            return m_transferCommandPool != VK_NULL_HANDLE ? m_transferCommandPool : m_graphicsCommandPool;
        case SubmitQueue::Compute:
            return m_graphicsCommandPool;
    }
    return m_graphicsCommandPool;
}

void VulkanDeviceContext::CreateDescriptorPool()
{
    VkDescriptorPool           descriptorPool;
    VkDescriptorPoolCreateInfo poolInfo = {};

    VkDescriptorPoolSize poolSizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                                         { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4096 },
                                         { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4096 },
                                         { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                                         { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                                         { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                                         { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                                         { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                                         { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                                         { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                                         { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags =
        VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    poolInfo.maxSets       = 1000 * sizeof(poolSizes) / sizeof(VkDescriptorPoolSize);
    poolInfo.poolSizeCount = sizeof(poolSizes) / sizeof(VkDescriptorPoolSize);
    poolInfo.pPoolSizes    = poolSizes;

    VkResult result = vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &descriptorPool);
    YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to create descriptor pool!");

    m_descriptorPools.push_back(descriptorPool);
}

VkSampler VulkanDeviceContext::GetSampler(SamplerReductionMode mode)
{
    const size_t slot = static_cast<size_t>(mode);
    YG_CORE_ASSERT(slot < m_samplers.size(), "Vulkan: SamplerReductionMode out of range");
    if (m_samplers[slot] != VK_NULL_HANDLE)
        return m_samplers[slot];

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter        = VK_FILTER_LINEAR;
    samplerInfo.minFilter        = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode       = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy    = 1.0f;
    samplerInfo.compareEnable    = VK_FALSE;
    samplerInfo.compareOp        = VK_COMPARE_OP_ALWAYS;
    samplerInfo.minLod           = 0.0f;
    samplerInfo.maxLod           = VK_LOD_CLAMP_NONE;
    samplerInfo.borderColor      = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

    VkSamplerReductionModeCreateInfo reductionInfo{};
    if (mode != SamplerReductionMode::None)
    {
        reductionInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO;
        reductionInfo.reductionMode =
            (mode == SamplerReductionMode::Max) ? VK_SAMPLER_REDUCTION_MODE_MAX : VK_SAMPLER_REDUCTION_MODE_MIN;
        samplerInfo.pNext = &reductionInfo;
    }

    VkResult result = vkCreateSampler(m_device, &samplerInfo, nullptr, &m_samplers[slot]);
    YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to create sampler!");
    return m_samplers[slot];
}

} // namespace Yogi
