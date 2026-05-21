#include "VulkanBuffer.h"

#include <volk.h>

namespace Yogi
{

Owner<IBuffer> IBuffer::Create(const BufferDesc& desc)
{
    return Owner<VulkanBuffer>::Create(desc);
}

VulkanBuffer::VulkanBuffer(const BufferDesc& desc) : m_size(desc.Size), m_usage(desc.Usage), m_access(desc.Access)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size        = m_size;
    bufferInfo.usage       = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (m_access == BufferAccess::Immutable)
    {
        bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    if (m_usage & BufferUsage::Vertex)
    {
        bufferInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    if (m_usage & BufferUsage::Index)
    {
        bufferInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }
    if (m_usage & BufferUsage::Uniform)
    {
        bufferInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }
    if (m_usage & BufferUsage::Storage)
    {
        bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }
    if (m_usage & BufferUsage::Staging)
    {
        bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }
    if (m_usage & BufferUsage::Indirect)
    {
        bufferInfo.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    }

    VulkanDeviceContext* context        = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());
    VkDevice             device         = context->GetVkDevice();
    VkPhysicalDevice     physicalDevice = context->GetVkPhysicalDevice();
    vkCreateBuffer(device, &bufferInfo, nullptr, &m_buffer);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, m_buffer, &memRequirements);

    // BDA requires the memory allocation to opt in too — VUID-VkMemoryAllocateInfo-flags-03331.
    VkMemoryAllocateFlagsInfo allocFlagsInfo{};
    allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext           = &allocFlagsInfo;
    allocInfo.allocationSize  = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits,
                                               physicalDevice,
                                               m_access == BufferAccess::Dynamic ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT :
                                                                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vkAllocateMemory(device, &allocInfo, nullptr, &m_memory);
    vkBindBufferMemory(device, m_buffer, m_memory, 0);

    VkBufferDeviceAddressInfo addrInfo{};
    addrInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addrInfo.buffer = m_buffer;
    m_deviceAddress = vkGetBufferDeviceAddress(device, &addrInfo);
    YG_CORE_ASSERT(m_deviceAddress != 0, "Vulkan: vkGetBufferDeviceAddress returned null");

    vkMapMemory(device, m_memory, 0, m_size, 0, &m_bufferMapped);
}

VulkanBuffer::~VulkanBuffer()
{
    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());
    VkDevice             device  = context->GetVkDevice();

    vkDeviceWaitIdle(device);
    vkUnmapMemory(device, m_memory);
    vkDestroyBuffer(device, m_buffer, nullptr);
    vkFreeMemory(device, m_memory, nullptr);
}

void VulkanBuffer::UpdateData(const void* data, uint64_t size, uint64_t offset)
{
    if (m_access == BufferAccess::Immutable)
    {
        YG_CORE_ERROR("Vulkan: Attempting to update immutable buffer!");
        return;
    }

    memcpy((char*)m_bufferMapped + offset, data, size);
}

} // namespace Yogi
