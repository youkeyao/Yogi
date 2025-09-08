#pragma once

#include "Renderer/RHI/IPipeline.h"
#include "Renderer/RHI/ITexture.h"

namespace Yogi
{

class YG_API Material
{
public:
    Material(const View<IPipeline>& pipeline);
    ~Material();

    void            SetPipeline(const View<IPipeline>& pipeline);
    View<IPipeline> GetPipeline() const { return m_pipeline; }

    int GetPositionOffset() const { return m_positionOffset; }
    int GetNormalOffset() const { return m_normalOffset; }
    int GetTexCoordOffset() const { return m_texcoordOffset; }
    int GetEntityOffset() const { return m_entityOffset; }

    void SetTexture(uint32_t index, const View<ITexture>& texture) { m_textures[index].second = texture; }
    const std::vector<std::pair<uint32_t, View<ITexture>>>& GetTextures() const { return m_textures; }

    uint8_t* GetData() const { return m_data; }

private:
    View<IPipeline> m_pipeline;
    int             m_positionOffset = -1;
    int             m_normalOffset   = -1;
    int             m_texcoordOffset = -1;
    int             m_entityOffset   = -1;
    uint8_t*        m_data           = nullptr;

    std::vector<std::pair<uint32_t, View<ITexture>>> m_textures;
};

} // namespace Yogi