#pragma once

namespace Yogi
{

enum class BufferUsage : uint8_t
{
    Vertex   = 1 << 0,
    Index    = 1 << 1,
    Uniform  = 1 << 2,
    Storage  = 1 << 3,
    Staging  = 1 << 4,
    Indirect = 1 << 5
};

enum class BufferAccess : uint8_t
{
    Dynamic,
    Immutable
};

struct BufferDesc
{
    uint32_t     Size;
    BufferUsage  Usage;
    BufferAccess Access;
    const void*  InitialData;
};

class YG_API IBuffer
{
public:
    virtual ~IBuffer() = default;

    // Buffer properties
    virtual uint32_t     GetSize() const   = 0;
    virtual BufferUsage  GetUsage() const  = 0;
    virtual BufferAccess GetAccess() const = 0;

    // Buffer operations
    virtual void UpdateData(const void* data, uint32_t size, uint32_t offset = 0) = 0;

    static Handle<IBuffer> Create(const BufferDesc& desc);
};

} // namespace Yogi
