#pragma once

#include "Core/EnumFlags.h"

namespace Yogi
{

enum class BufferUsage : uint16_t
{
    Vertex   = 1 << 0,
    Index    = 1 << 1,
    Uniform  = 1 << 2,
    Storage  = 1 << 3,
    Staging  = 1 << 4,
    Indirect = 1 << 5
};

YG_ENABLE_ENUM_FLAGS(BufferUsage);

struct BufferDesc
{
    uint64_t    Size;
    BufferUsage Usage;
};

class YG_API IBuffer
{
public:
    virtual ~IBuffer() = default;

    // Buffer properties
    virtual uint64_t    GetSize() const  = 0;
    virtual BufferUsage GetUsage() const = 0;

    virtual uint64_t GetDeviceAddress() const = 0;

    virtual void* GetMappedPtr() const = 0;

    static Owner<IBuffer> Create(const BufferDesc& desc);
};

template <>
template <typename... Args>
inline Owner<IBuffer> Owner<IBuffer>::Create(Args&&... args)
{
    return IBuffer::Create(std::forward<Args>(args)...);
}

} // namespace Yogi
