#pragma once

#include "Renderer/RHI/IBuffer.h"
#include "Renderer/RHI/ITextureView.h"

namespace Yogi
{

enum class ShaderResourceType : uint8_t
{
    StorageBuffer,
    StorageTexture,
    SampledTexture,
    Sampler,
};

enum class SamplerReductionMode : uint8_t
{
    None,
    Min,
    Max,
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
YG_ENABLE_ENUM_FLAGS(ShaderStage);

struct ShaderResourceAttribute
{
    int                Binding;
    int                Count;
    ShaderResourceType Type;
    ShaderStage        Stage;
};

struct ImmutableSamplerDesc
{
    int                  Binding;
    SamplerReductionMode Reduction;
};

class YG_API IShaderResourceBinding
{
public:
    virtual ~IShaderResourceBinding() = default;

    virtual void BindBuffer(const IBuffer* buffer, int binding, int slot = 0)         = 0;
    virtual void BindTextureView(const ITextureView* view, int binding, int slot = 0) = 0;

    const std::vector<ShaderResourceAttribute>& GetLayout() const { return m_layout; }
    const std::vector<ImmutableSamplerDesc>&    GetImmutableSamplers() const { return m_immutableSamplers; }

    static Owner<IShaderResourceBinding> Create(const std::vector<ShaderResourceAttribute>& shaderResourceLayout,
                                                const std::vector<ImmutableSamplerDesc>&    immutableSamplers = {});

protected:
    std::vector<ShaderResourceAttribute> m_layout;
    std::vector<ImmutableSamplerDesc>    m_immutableSamplers;
};

template <>
template <typename... Args>
inline Owner<IShaderResourceBinding> Owner<IShaderResourceBinding>::Create(Args&&... args)
{
    return IShaderResourceBinding::Create(std::forward<Args>(args)...);
}

} // namespace Yogi
