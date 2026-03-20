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

class YG_API IShaderResourceBinding
{
public:
    virtual ~IShaderResourceBinding() = default;

    virtual void BindBuffer(const Ref<IBuffer>& buffer, int binding, int slot = 0)    = 0;
    virtual void BindTexture(const Ref<ITexture>& texture, int binding, int slot = 0) = 0;

    const std::vector<ShaderResourceAttribute>& GetLayout() const { return m_layout; }

    static Handle<IShaderResourceBinding> Create(const std::vector<ShaderResourceAttribute>& shaderResourceLayout);

protected:
    std::vector<ShaderResourceAttribute> m_layout;
};

template <>
template <typename... Args>
inline Handle<IShaderResourceBinding> Handle<IShaderResourceBinding>::Create(Args&&... args)
{
    return IShaderResourceBinding::Create(std::forward<Args>(args)...);
}

} // namespace Yogi
