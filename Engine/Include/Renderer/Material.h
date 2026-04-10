#pragma once

#include "Renderer/RHI/IPipeline.h"
#include "Renderer/RHI/ITexture.h"

namespace Yogi
{

class YG_API Material
{
public:
    struct MaterialPass
    {
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