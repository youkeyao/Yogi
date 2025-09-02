#pragma once

#include "Renderer/RHI/IDeviceContext.h"
#include "Core/Application.h"
#include "VulkanUtils.h"

namespace Yogi
{

class VulkanDeviceContext : public IDeviceContext
{
public:
    VulkanDeviceContext();
    virtual ~VulkanDeviceContext();

    void WaitIdle() override;

    VkDescriptorSet AllocateVkDescriptorSet(VkDescriptorSetLayout layout);

    inline VkInstance       GetVkInstance() const { return m_instance; }
    inline VkPhysicalDevice GetVkPhysicalDevice() const { return m_physicalDevice; }
    inline VkDevice         GetVkDevice() const { return m_device; }
    inline VkCommandPool    GetVkCommandPool() const { return m_commandPool; }

    inline VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
    inline VkQueue GetTransferQueue() const { return m_transferQueue; }

private:
    void CreateVkInstance();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateCommandPool();
    void CreateDescriptorPool();

protected:
    const std::vector<const char*> DeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                       VK_KHR_MAINTENANCE1_EXTENSION_NAME,
                                                       VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME};

    VkInstance       m_instance;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice         m_device;

    VkCommandPool m_commandPool;
    VkQueue       m_graphicsQueue;
    VkQueue       m_transferQueue;

    std::vector<VkDescriptorPool> m_descriptorPools;

#ifdef YG_DEBUG
    const std::vector<const char*> ValidationLayers = {"VK_LAYER_KHRONOS_validation"};
    VkDebugUtilsMessengerEXT       m_debugMessenger;
#endif
};

} // namespace Yogi
