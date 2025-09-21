#pragma once

#include "Renderer/RHI/IPipeline.h"
#include "Renderer/RHI/ITexture.h"

namespace Yogi
{

class YG_API Material
{
public:
    ~Material() = default;

    inline void           SetPipeline(const Ref<IPipeline>& pipeline) { m_pipeline = pipeline; }
    inline Ref<IPipeline> GetPipeline() const { return m_pipeline; }

    inline int GetPositionOffset() const { return m_positionOffset; }
    inline int GetNormalOffset() const { return m_normalOffset; }
    inline int GetTexCoordOffset() const { return m_texcoordOffset; }
    inline int GetEntityOffset() const { return m_entityOffset; }

    inline void SetTexture(uint32_t index, const Ref<ITexture>& texture) { m_textures[index].second = texture; }
    inline const std::vector<std::pair<uint32_t, Ref<ITexture>>>& GetTextures() const { return m_textures; }

    inline std::vector<uint8_t>& GetData() { return m_data; }

    static Handle<Material> Create(const Ref<IPipeline>& pipeline);

protected:
    friend Handle<Material>;
    Material(const Ref<IPipeline>& pipeline);

private:
    Ref<IPipeline>       m_pipeline;
    std::vector<uint8_t> m_data;

    int m_positionOffset = -1;
    int m_normalOffset   = -1;
    int m_texcoordOffset = -1;
    int m_entityOffset   = -1;

    std::vector<std::pair<uint32_t, Ref<ITexture>>> m_textures;
};

} // namespace Yogi