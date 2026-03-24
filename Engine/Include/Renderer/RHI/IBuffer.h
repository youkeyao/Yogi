#pragma once

#include "Core/EnumFlags.h"

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

YG_ENABLE_ENUM_FLAGS(BufferUsage);

enum class BufferAccess : uint8_t
{
    Dynamic,
    Immutable
};

struct BufferDesc
{
    uint64_t     Size;
    BufferUsage  Usage;
    BufferAccess Access;
};

class YG_API IBuffer
{
public:
    virtual ~IBuffer() = default;

    // Buffer properties
    virtual uint64_t     GetSize() const   = 0;
    virtual BufferUsage  GetUsage() const  = 0;
    virtual BufferAccess GetAccess() const = 0;

    // Buffer operations
    virtual void UpdateData(const void* data, uint64_t size, uint64_t offset = 0) = 0;

    static Owner<IBuffer> Create(const BufferDesc& desc);
};

template <>
template <typename... Args>
inline Owner<IBuffer> Owner<IBuffer>::Create(Args&&... args)
{
    return IBuffer::Create(std::forward<Args>(args)...);
}

} // namespace Yogi
