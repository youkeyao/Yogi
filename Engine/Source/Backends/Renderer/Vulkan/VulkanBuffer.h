#pragma once

#include "Renderer/RHI/IBuffer.h"
#include "VulkanDeviceContext.h"

namespace Yogi
{

class VulkanBuffer : public IBuffer
{
public:
    VulkanBuffer(const BufferDesc& desc);
    virtual ~VulkanBuffer();

    inline uint64_t    GetSize() const override { return m_size; }
    inline BufferUsage GetUsage() const override { return m_usage; }
    inline uint64_t    GetDeviceAddress() const override { return m_deviceAddress; }

    inline void* GetMappedPtr() const override { return m_bufferMapped; }

    inline VkBuffer GetVkBuffer() const { return m_buffer; }

private:
    VkBuffer        m_buffer        = VK_NULL_HANDLE;
    VkDeviceMemory  m_memory        = VK_NULL_HANDLE;
    VkDeviceAddress m_deviceAddress = 0;

    uint64_t    m_size         = 0;
    BufferUsage m_usage        = {};
    void*       m_bufferMapped = nullptr;
};

} // namespace Yogi
