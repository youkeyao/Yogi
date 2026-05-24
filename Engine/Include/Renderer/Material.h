#pragma once

#include "Renderer/RHI/IPipeline.h"
#include "Renderer/RHI/ITexture.h"

namespace Yogi
{

// Serialization-friendly description of how a pipeline was built. Stored on the material so the
// serializer can round-trip a material without reflecting back through the opaque IPipeline.
//
// Pipeline rendering state used to live in a referenced IRenderPass asset; with
// dynamic rendering it's flattened into the pipeline itself: the attachment
// formats + sample count are all the GPU needs at PSO compile time.
struct PipelineData
{
    std::vector<std::string>             ShaderKeys;
    std::vector<VertexAttribute>         VertexLayout;
    std::vector<ShaderResourceAttribute> ShaderResourceLayout;
    std::vector<PushConstantRange>       PushConstantRanges;
    std::vector<ITexture::Format>        ColorFormats;
    ITexture::Format                     DepthFormat = ITexture::Format::NONE;
    SampleCountFlagBits                  Samples     = SampleCountFlagBits::Count1;
    PrimitiveTopology                    Topology    = PrimitiveTopology::TriangleList;
};

class YG_API Material
{
public:
    struct MaterialPass
    {
        PipelineData                PipelineInfo;
        WRef<IPipeline>             Pipeline;
        // Optional alternate pipeline used by ForwardRenderSystem's LATE
        // (post-Hi-Z-build) render. Lets meshlet pipelines ship two SPIR-V
        // task-shader variants -- EARLY (no Hi-Z) and LATE (with Hi-Z) -- while
        // sharing the same shader resource layout. nullptr means "no two-phase
        // variant; LATE render reuses Pipeline" (2D / editor / non-meshlet paths).
        WRef<IPipeline>             LatePipeline;
        std::vector<WRef<ITexture>> Textures;
    };

public:
    Material()  = default;
    ~Material() = default;

    void SetPass(uint32_t index, const MaterialPass& pass)
    {
        if (index < m_passes.size())
            m_passes[index] = pass;
    }
    void AddPass(const MaterialPass& pass) { m_passes.push_back(pass); }
    void RemovePass(uint32_t index)
    {
        if (index < m_passes.size())
            m_passes.erase(m_passes.begin() + index);
    }

    const std::vector<MaterialPass>& GetPasses() const { return m_passes; }

private:
    std::vector<MaterialPass> m_passes;
};

} // namespace Yogi