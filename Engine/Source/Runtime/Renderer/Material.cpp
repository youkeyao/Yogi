#include "Renderer/Material.h"

namespace Yogi
{

void Material::AddPass(const Ref<IPipeline>& pipeline)
{
    MaterialPass pass;
    pass.Pipeline = pipeline;

    auto&    vertexLayout = pipeline->GetDesc().VertexLayout;
    uint32_t stride       = vertexLayout.back().Offset + vertexLayout.back().Size;
    pass.Data.resize(stride);
    for (auto& element : vertexLayout)
    {
        if (element.Usage == AttributeUsage::Position)
        {
            pass.PositionOffset = element.Offset;
        }
        else if (element.Usage == AttributeUsage::Normal)
        {
            pass.NormalOffset = element.Offset;
        }
        else if (element.Usage == AttributeUsage::Texcoord)
        {
            pass.TexCoordOffset = element.Offset;
        }
        else if (element.Usage == AttributeUsage::EntityID)
        {
            pass.EntityOffset = element.Offset;
        }
        else if (element.Usage == AttributeUsage::TextureID)
        {
            pass.Textures.push_back({ element.Offset, nullptr });
        }
    }

    m_passes.push_back(pass);
}

} // namespace Yogi