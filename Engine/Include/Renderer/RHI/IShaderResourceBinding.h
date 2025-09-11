#pragma once

#include "Renderer/RHI/IBuffer.h"
#include "Renderer/RHI/ITexture.h"

namespace Yogi
{

enum class ShaderResourceType : uint8_t
{
    Buffer,
    Texture
};

enum class ShaderStage : uint8_t
{
    Vertex,
    Geometry,
    Fragment,
    Compute
};

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

    static Handle<IShaderResourceBinding> Create(const std::vector<ShaderResourceAttribute>& shaderResourceLayout);
};

} // namespace Yogi
