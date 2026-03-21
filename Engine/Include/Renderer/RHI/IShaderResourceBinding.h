#pragma once

#include "Renderer/RHI/IBuffer.h"
#include "Renderer/RHI/ITexture.h"

namespace Yogi
{

enum class ShaderResourceType : uint8_t
{
    StorageBuffer,
    Texture
};

enum class ShaderStage : uint8_t
{
    Vertex   = 1 << 0,
    Geometry = 1 << 1,
    Fragment = 1 << 2,
    Compute  = 1 << 3,
    Task     = 1 << 4,
    Mesh     = 1 << 5
};

inline ShaderStage operator|(ShaderStage a, ShaderStage b)
{
    return static_cast<ShaderStage>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
inline ShaderStage operator&(ShaderStage a, ShaderStage b)
{
    return static_cast<ShaderStage>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

struct ShaderResourceAttribute
{
    int                Binding;
    int                Count;
    ShaderResourceType Type;
    ShaderStage        Stage;
};

struct PushConstantRange
{
    ShaderStage Stage;
    uint32_t    Offset;
    uint32_t    Size;
};

class YG_API IShaderResourceBinding
{
public:
    virtual ~IShaderResourceBinding() = default;

    virtual void BindBuffer(const Ref<IBuffer>& buffer, int binding, int slot = 0)    = 0;
    virtual void BindTexture(const Ref<ITexture>& texture, int binding, int slot = 0) = 0;

    const std::vector<ShaderResourceAttribute>& GetLayout() const { return m_layout; }
    const std::vector<PushConstantRange>&       GetPushConstantRanges() const { return m_pushConstantRanges; }

    static Owner<IShaderResourceBinding> Create(const std::vector<ShaderResourceAttribute>& shaderResourceLayout,
                                                const std::vector<PushConstantRange>&       pushConstantRanges = {});

protected:
    std::vector<ShaderResourceAttribute> m_layout;
    std::vector<PushConstantRange>       m_pushConstantRanges;
};

template <>
template <typename... Args>
inline Owner<IShaderResourceBinding> Owner<IShaderResourceBinding>::Create(Args&&... args)
{
    return IShaderResourceBinding::Create(std::forward<Args>(args)...);
}

} // namespace Yogi
