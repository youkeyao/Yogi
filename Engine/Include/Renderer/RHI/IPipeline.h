#pragma once

#include "Renderer/RHI/ITexture.h"
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
    std::string Name;
    uint32_t    Offset;
    uint32_t    Size;
    Format      Format;
};

struct PushConstantRange
{
    ShaderStage Stage;
    uint32_t    Offset;
    uint32_t    Size;
};

enum class CullMode : uint8_t
{
    None,
    Back,
    Front,
};

enum class CompareOp : uint8_t
{
    Never = 0,
    Less,
    Equal,
    LessOrEqual,
    Greater,
    NotEqual,
    GreaterOrEqual,
    Always,
};

enum class StencilOp : uint8_t
{
    Keep = 0,
    Zero,
    Replace,
    IncrementClamp,
    DecrementClamp,
    Invert,
    IncrementWrap,
    DecrementWrap,
};

struct StencilFaceDesc
{
    CompareOp CompareFn   = CompareOp::Always;
    StencilOp FailOp      = StencilOp::Keep;
    StencilOp DepthFailOp = StencilOp::Keep;
    StencilOp PassOp      = StencilOp::Keep;
};

struct StencilDesc
{
    bool            Enable    = false;
    uint8_t         ReadMask  = 0xFF;
    uint8_t         WriteMask = 0xFF;
    uint8_t         Reference = 0;
    StencilFaceDesc Front     = {};
    StencilFaceDesc Back      = {};
};

enum class BlendFactor : uint8_t
{
    Zero = 0,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
};

enum class BlendOp : uint8_t
{
    Add = 0,
    Subtract,
    ReverseSubtract,
    Min,
    Max,
};

enum class ColorWriteMask : uint8_t
{
    None = 0,
    R    = 1 << 0,
    G    = 1 << 1,
    B    = 1 << 2,
    A    = 1 << 3,
    All  = R | G | B | A,
};
YG_ENABLE_ENUM_FLAGS(ColorWriteMask);

struct BlendDesc
{
    BlendFactor SrcFactor = BlendFactor::One;
    BlendFactor DstFactor = BlendFactor::Zero;
    BlendOp     Op        = BlendOp::Add;
};

struct ColorTargetDesc
{
    Format         Format      = Format::NONE;
    BlendDesc      ColorBlend  = {};
    BlendDesc      AlphaBlend  = {};
    bool           EnableBlend = false;
    ColorWriteMask WriteMask   = ColorWriteMask::All;
};

struct PipelineDesc
{
    std::vector<const ShaderDesc*> Shaders;
    std::vector<VertexAttribute>   VertexLayout;
    const IShaderResourceBinding*  ResourceBinding = nullptr;
    std::vector<PushConstantRange> PushConstantRanges;

    std::vector<ColorTargetDesc> ColorTargets;
    Format                       DepthFormat = Format::NONE;
    SampleCountFlagBits          Samples     = SampleCountFlagBits::Count1;
    PrimitiveTopology            Topology    = PrimitiveTopology::TriangleList;
    PipelineType                 Type        = PipelineType::Graphics;
    CullMode                     Cull        = CullMode::Back;

    // Depth state
    bool      DepthTestEnable  = true;
    bool      DepthWriteEnable = true;
    CompareOp DepthCompareOp   = CompareOp::Less;

    // Stencil state
    StencilDesc Stencil = {};
};

class YG_API IPipeline
{
public:
    virtual ~IPipeline() = default;

    virtual PipelineType GetType() const = 0;

    static Owner<IPipeline> Create(const PipelineDesc& desc);
};

template <>
template <typename... Args>
inline Owner<IPipeline> Owner<IPipeline>::Create(Args&&... args)
{
    return IPipeline::Create(std::forward<Args>(args)...);
}

} // namespace Yogi
