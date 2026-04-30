#pragma once

#include "Renderer/RHI/IPipeline.h"
#include "Renderer/RHI/ITexture.h"

namespace Yogi
{

// Serialization-friendly description of how a pipeline was built. Stored on the material so the
// serializer can round-trip a material without reflecting back through the opaque IPipeline.
struct PipelineData
{
    std::vector<std::string>             ShaderKeys;
    std::vector<VertexAttribute>         VertexLayout;
    std::vector<ShaderResourceAttribute> ShaderResourceLayout;
    std::vector<PushConstantRange>       PushConstantRanges;
    std::string                          RenderPassKey;
    int                                  SubPassIndex = 0;
    PrimitiveTopology                    Topology     = PrimitiveTopology::TriangleList;
};

class YG_API Material
{
public:
    struct MaterialPass
    {
        PipelineData                PipelineInfo;
        WRef<IPipeline>             Pipeline;
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