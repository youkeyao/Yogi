#include "Renderer/Material.h"

namespace Yogi
{

void Material::SetPipeline(const Ref<IPipeline>& pipeline)
{
    m_pipeline = pipeline;

    auto&    vertexLayout = m_pipeline->GetDesc().VertexLayout;
    uint32_t stride       = vertexLayout.back().Offset + vertexLayout.back().Size;
    m_data.resize(stride);
    for (auto& element : vertexLayout)
    {
        if (element.Name == "a_Position")
        {
            m_positionOffset = element.Offset;
        }
        else if (element.Name == "a_Normal")
        {
            m_normalOffset = element.Offset;
        }
        else if (element.Name == "a_TexCoord")
        {
            m_texcoordOffset = element.Offset;
        }
        else if (element.Name == "a_EntityID")
        {
            m_entityOffset = element.Offset;
        }
        else if (element.Name.substr(0, 4) == "TEX_")
        {
            // m_textures.push_back({ element.Offset, nullptr });
        }
    }
}

} // namespace Yogi