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

    uint32_t     GetSize() const override { return m_size; }
    BufferUsage  GetUsage() const override { return m_usage; }
    BufferAccess GetAccess() const override { return m_access; }

    void UpdateData(const void* data, uint32_t size, uint32_t offset = 0) override;

    VkBuffer GetVkBuffer() const { return m_buffer; }

private:
    VkBuffer       m_buffer;
    VkDeviceMemory m_memory;

    uint32_t     m_size;
    BufferUsage  m_usage;
    BufferAccess m_access;
    void*        m_bufferMapped;
};

} // namespace Yogi
