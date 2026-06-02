#include "Renderer/BindlessTextureManager.h"
#include "Renderer/ShaderData.h"

namespace Yogi
{

namespace
{
constexpr int kBindingSampler         = 0;
constexpr int kBindingSamplerMax      = 1;
constexpr int kBindingSampledTextures = 2;
} // namespace

BindlessTextureManager* BindlessTextureManager::s_instance = nullptr;

BindlessTextureManager::BindlessTextureManager()
{
    const ShaderStage stages = ShaderStage::Vertex | ShaderStage::Fragment | ShaderStage::Compute | ShaderStage::Task |
        ShaderStage::Mesh | ShaderStage::Geometry;

    m_srb = IShaderResourceBinding::Create(
        std::vector<ShaderResourceAttribute>{
            ShaderResourceAttribute{ kBindingSampler, 1, ShaderResourceType::Sampler, stages },
            ShaderResourceAttribute{ kBindingSamplerMax, 1, ShaderResourceType::Sampler, stages },
            ShaderResourceAttribute{
                kBindingSampledTextures, MAX_TEXTURES, ShaderResourceType::SampledTexture, stages } },
        std::vector<ImmutableSamplerDesc>{ ImmutableSamplerDesc{ kBindingSampler, SamplerReductionMode::None },
                                           ImmutableSamplerDesc{ kBindingSamplerMax, SamplerReductionMode::Max } });

    EnsureDefaultWhite();
}

BindlessTextureManager::~BindlessTextureManager()
{
    m_viewToSlot.clear();
    m_freeList.clear();
    m_defaultWhiteView = nullptr;
    m_defaultWhite     = nullptr;
    m_srb              = nullptr;
}

void BindlessTextureManager::EnsureDefaultWhite()
{
    if (m_defaultWhite)
        return;

    TextureDesc desc{};
    desc.Width         = 1;
    desc.Height        = 1;
    desc.MipLevels     = 1;
    desc.Format        = ITexture::Format::R8G8B8A8_UNORM;
    desc.Usage         = ITexture::Usage::Texture2D;
    desc.NumSamples    = SampleCountFlagBits::Count1;
    m_defaultWhite     = ITexture::Create(desc);
    m_defaultWhiteView = ITextureView::Create(WRef<ITexture>::Create(m_defaultWhite), TextureViewDesc{});

    const uint8_t whiteRGBA[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
    m_defaultWhiteView->SetData(const_cast<uint8_t*>(whiteRGBA), sizeof(whiteRGBA));

    m_srb->BindTextureView(m_defaultWhiteView.Get(), kBindingSampledTextures, 0);
    m_viewToSlot[m_defaultWhiteView.Get()] = 0u;
}

uint32_t BindlessTextureManager::Register(const ITextureView* view)
{
    YG_CORE_ASSERT(view, "BindlessTextureManager::Register called with null view");
    BindlessTextureManager& self = Get();

    auto it = self.m_viewToSlot.find(view);
    if (it != self.m_viewToSlot.end())
        return it->second;

    uint32_t slot;
    if (!self.m_freeList.empty())
    {
        slot = self.m_freeList.back();
        self.m_freeList.pop_back();
    }
    else
    {
        if (self.m_nextSlot >= MAX_TEXTURES)
        {
            YG_CORE_WARN("BindlessTextureManager: pool exhausted ({0} slots), falling back to slot 0 "
                         "(default white). Raise MAX_TEXTURES in ShaderData.h.",
                         MAX_TEXTURES);
            return 0u;
        }
        slot = self.m_nextSlot++;
    }

    self.m_srb->BindTextureView(view, kBindingSampledTextures, static_cast<int>(slot));
    self.m_viewToSlot[view] = slot;
    return slot;
}

void BindlessTextureManager::Unregister(uint32_t slot)
{
    if (slot == INVALID_SLOT || slot == 0u)
        return; // slot 0 (default white) is never released

    BindlessTextureManager& self = Get();
    for (auto it = self.m_viewToSlot.begin(); it != self.m_viewToSlot.end(); ++it)
    {
        if (it->second == slot)
        {
            self.m_viewToSlot.erase(it);
            break;
        }
    }
    self.m_freeList.push_back(slot);
}

uint32_t BindlessTextureManager::Find(const ITextureView* view)
{
    BindlessTextureManager& self = Get();
    auto                    it   = self.m_viewToSlot.find(view);
    return it == self.m_viewToSlot.end() ? INVALID_SLOT : it->second;
}

} // namespace Yogi
