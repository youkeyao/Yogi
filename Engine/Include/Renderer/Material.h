#pragma once

#include "Renderer/RHI/IPipeline.h"
#include "Renderer/RHI/ITexture.h"

namespace Yogi
{

class YG_API Material
{
    struct MaterialPass
    {
        Ref<IPipeline>                                  Pipeline;
        std::vector<uint8_t>                            Data;
        std::vector<std::pair<uint32_t, Ref<ITexture>>> Textures;
        int                                             PositionOffset = -1;
        int                                             NormalOffset   = -1;
        int                                             TexCoordOffset = -1;
        int                                             EntityOffset   = -1;
    };

public:
    Material()  = default;
    ~Material() = default;

    void SetPass(uint32_t index, const MaterialPass& pass)
    {
        if (index < m_passes.size())
            m_passes[index] = pass;
    }
    void AddPass(const Ref<IPipeline>& pipeline);
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