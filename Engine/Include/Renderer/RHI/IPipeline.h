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
    ShaderStage           Stage; // Shader stage (vertex, fragment, compute)
    std::vector<uint32_t> Code;  // SPIR-V bytecode
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
    std::vector<ShaderDesc>      Shaders;
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

    const std::vector<VertexAttribute>& GetVertexLayout() const { return m_vertexLayout; }

    static Handle<IPipeline> Create(const PipelineDesc& desc);

protected:
    std::vector<VertexAttribute> m_vertexLayout;
};

} // namespace Yogi
