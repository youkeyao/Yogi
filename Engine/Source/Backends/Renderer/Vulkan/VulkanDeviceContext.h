#pragma once

#include "Renderer/RHI/IDeviceContext.h"
#include "Renderer/RHI/ICommandBuffer.h"
#include "Renderer/RHI/IShaderResourceBinding.h"
#include "Renderer/RHI/ITexture.h"
#include "Renderer/RHI/ITextureView.h"
#include "Core/Application.h"
#include "VulkanUtils.h"

namespace Yogi
{

class VulkanDeviceContext : public IDeviceContext
{
public:
    VulkanDeviceContext(const Window* window);
    virtual ~VulkanDeviceContext();

    void WaitIdle() override;

    VkDescriptorSet AllocateVkDescriptorSet(VkDescriptorSetLayout layout, uint32_t variableDescriptorCount = 0);

    inline VkInstance       GetVkInstance() const { return m_instance; }
    inline VkSurfaceKHR     GetVkSurface() const { return m_surface; }
    inline VkPhysicalDevice GetVkPhysicalDevice() const { return m_physicalDevice; }
    inline VkDevice         GetVkDevice() const { return m_device; }
    inline VkDescriptorPool GetVkDescriptorPool() const { return m_descriptorPools.back(); }

    VkCommandPool GetVkCommandPoolForQueue(SubmitQueue queue) const;

    VkSampler GetSampler(SamplerReductionMode mode = SamplerReductionMode::None);

    inline VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
    inline VkQueue GetTransferQueue() const { return m_transferQueue; }

    static PFN_vkVoidFunction VkLoadFunction(VkInstance instance, const char* funcName);

private:
    void CreateVkInstance();
    void CreateVkSurface(const Window* window);
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateCommandPools();
    void CreateDescriptorPool();

protected:
    std::vector<const char*> DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                  VK_KHR_MAINTENANCE1_EXTENSION_NAME,
                                                  VK_EXT_MESH_SHADER_EXTENSION_NAME,
                                                  VK_KHR_8BIT_STORAGE_EXTENSION_NAME,
                                                  VK_KHR_16BIT_STORAGE_EXTENSION_NAME };

    VkInstance       m_instance;
    VkSurfaceKHR     m_surface        = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice         m_device;

    VkCommandPool m_graphicsCommandPool = VK_NULL_HANDLE;
    VkCommandPool m_transferCommandPool = VK_NULL_HANDLE;
    VkQueue       m_graphicsQueue;
    VkQueue       m_transferQueue;

    std::vector<VkDescriptorPool> m_descriptorPools;

    std::array<VkSampler, 3> m_samplers = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };

#ifdef YG_DEBUG
    const std::vector<const char*> ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
    VkDebugUtilsMessengerEXT       m_debugMessenger;
#endif
};

} // namespace Yogi
