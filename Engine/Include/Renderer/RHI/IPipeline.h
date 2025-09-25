#pragma once

#include "Renderer/RHI/ITexture.h"
#include "Renderer/RHI/IRenderPass.h"
#include "Renderer/RHI/IShaderResourceBinding.h"

namespace Yogi
{

enum class PrimitiveTopology : uint8_t
{
    TriangleList,
    TriangleStrip,
    LineList,
    PointList
};

enum class ShaderElementType : uint8_t
{
    None = 0,
    Float,
    Float2,
    Float3,
    Float4,
    Mat3,
    Mat4,
    Int,
    Int2,
    Int3,
    Int4,
    Bool
};

enum class PipelineType : uint8_t
{
    Graphics,
    Compute
};

struct ShaderDesc
{
    ShaderStage          Stage; // Shader stage (vertex, fragment, compute)
    std::vector<uint8_t> Code;  // SPIR-V bytecode
};

struct VertexAttribute
{
    std::string       Name;
    uint32_t          Offset;
    uint32_t          Size;
    ShaderElementType Type;
};

struct PipelineDesc
{
    std::vector<Ref<ShaderDesc>> Shaders;
    std::vector<VertexAttribute> VertexLayout;
    Ref<IShaderResourceBinding>  ShaderResourceBinding;
    Ref<IRenderPass>             RenderPass;
    int                          SubPassIndex;
    PrimitiveTopology            Topology = PrimitiveTopology::TriangleList;
};

class YG_API IPipeline
{
public:
    virtual ~IPipeline() = default;

    const PipelineDesc& GetDesc() const { return m_desc; }

    static Handle<IPipeline> Create(const PipelineDesc& desc);

protected:
    PipelineDesc m_desc;
};

template <>
template <typename... Args>
inline Handle<IPipeline> Handle<IPipeline>::Create(Args&&... args)
{
    return IPipeline::Create(std::forward<Args>(args)...);
}

} // namespace Yogi
